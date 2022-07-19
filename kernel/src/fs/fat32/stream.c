#include <fs/fat32/stream.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <fs/fat32/fat32.h>
#include <fs/fat32/dir.h>

#include <preos_renderer.h>
#include <cstr.h>

FAT_READ_STREAM* FatOpenReadStream(UINT32 InitialCluster, UINT32 ReadSize, void* ReadBuffer, PARTITION_INSTANCE* Partition) {
	if (InitialCluster < 2 || !ValidatePartition(Partition)) return NULL;
	FAT_READ_STREAM* Stream = kmalloc(sizeof(FAT_READ_STREAM));
	if (!Stream) SET_SOD_MEMORY_MANAGEMENT;
	Stream->Set = TRUE;
	Stream->InitialRead = FALSE;
	// double buffer sizes for some special cases e.g. Reading Long File Name Entries
	Stream->FatBuffer = kmalloc(Partition->PartitionInformation.BytesInCluster); // equivalent of * 512 * 2
	
	Stream->SecondBuffer = kmalloc(Partition->PartitionInformation.BytesInCluster);
	if (!ReadSize) ReadSize = 0xFFFFFFFF;
	Stream->ReadSize = ReadSize;
	Stream->ReadBuffer = ReadBuffer;
	Stream->RemainingRead = ReadSize;
	if (!Stream->FatBuffer || !Stream->SecondBuffer) SET_SOD_MEMORY_MANAGEMENT;
	Stream->InitialCluster = InitialCluster;
	Stream->Partition = Partition;
	Stream->ProtectedClusterSize = Partition->PartitionInformation.BytesInCluster;
	Stream->ClusterSize = Partition->PartitionInformation.BytesInCluster << 1;

	
	UINT32 FatCluster = (InitialCluster << 2) / Stream->ProtectedClusterSize; // equivalent of << 2 (* 4)
	Stream->FatCluster = FatCluster;
	struct FAT32_FS_DESCRIPTOR* fsdesc = Partition->FsInfo;
	if (KERNEL_ERROR(FsReadPartitionSectors(Partition,
		fsdesc->reserved_area_size + FatCluster * Partition->PartitionInformation.SectorsInCluster,
		Partition->PartitionInformation.SectorsInCluster, Stream->FatBuffer
	))) {
		FatCloseReadStream(Stream);
		return NULL;
	}
	/*GP_clear_screen(0);
	GP_draw_sf_text(to_hstring64(Partition->cluster_size), 0xffffff, 20, 20);
	while (1);*/
	Stream->FatIndex = ((InitialCluster << 2) % Stream->ProtectedClusterSize) >> 2; // Additionnal Index
	Stream->MaxIndex = Stream->ProtectedClusterSize >> 2; // / 4
	return Stream;
}

BOOL FatCloseReadStream(FAT_READ_STREAM* ReadStream) {
	if (!ReadStream->Set) return FALSE;
	free(ReadStream->FatBuffer, kproc);
	free(ReadStream->SecondBuffer, kproc);
	free(ReadStream, kproc);
	return TRUE;
}

void* GetBuffer(FAT_READ_STREAM* Stream, BOOL* Copy) {
	void* Buffer = Stream->SecondBuffer;
	if (Stream->ReadBuffer) {
		if (Stream->RemainingRead >= Stream->Partition->PartitionInformation.BytesInCluster) {
			Buffer = Stream->ReadBuffer;
			Stream->ReadBuffer += Stream->Partition->PartitionInformation.BytesInCluster;
			Stream->RemainingRead -= Stream->Partition->PartitionInformation.BytesInCluster;
			*Copy = FALSE;
		}
		else {
			*Copy = TRUE;
		}
	}
	return Buffer;
}

void* FatReadInitialCluster(FAT_READ_STREAM* ReadStream, UINT32* ClusterIndex, void* Buffer, BOOL Copy) {
	struct FAT32_FS_DESCRIPTOR* fsdesc = ReadStream->Partition->FsInfo;
	UINT32 Cluster = ReadStream->InitialCluster - 2;
	FsReadPartition(ReadStream->Partition, ReadStream->InitialCluster, 1, Buffer);

	ReadStream->InitialRead = TRUE;

	if (ClusterIndex) {
		*ClusterIndex = ReadStream->InitialCluster;
	}

	if (Copy) {
		memcpy(ReadStream->ReadBuffer, Buffer, ReadStream->RemainingRead);
		ReadStream->ReadBuffer += ReadStream->RemainingRead;
		ReadStream->RemainingRead = 0;
		Buffer = ReadStream->ReadBuffer;
	}
	return Buffer;
}
UINT32 GetFatCluster(FAT_READ_STREAM* Stream) {
	struct FAT32_FS_DESCRIPTOR* fsdesc = Stream->Partition->FsInfo;

	if (Stream->FatIndex == Stream->MaxIndex) {
		FsReadPartitionSectors(Stream->Partition, fsdesc->reserved_area_size + Stream->FatCluster * Stream->Partition->PartitionInformation.SectorsInCluster, Stream->Partition->PartitionInformation.SectorsInCluster, Stream->FatBuffer);
		Stream->FatCluster++;
	}

	UINT32 Cluster = Stream->FatBuffer[Stream->FatIndex] & FAT32_CLUSTER_MASK;
	if (Cluster >= FAT32_END_OF_CHAIN) return 0; // Read stream ended and must be closed
	if (Cluster < 2) return 0; // Invalid read value
	Stream->FatIndex++;
	return Cluster;
}
void* FatReadNextCluster(FAT_READ_STREAM* ReadStream, UINT32* ClusterIndex) {
	if (!ReadStream || !ReadStream->Set || !ReadStream->RemainingRead) return NULL;
	struct FAT32_FS_DESCRIPTOR* fsdesc = ReadStream->Partition->FsInfo;
	BOOL Copy = FALSE;
	void* Buffer = GetBuffer(ReadStream, &Copy);

	if (!ReadStream->InitialRead) {
		return FatReadInitialCluster(ReadStream, ClusterIndex, Buffer, Copy);
	}
	UINT32 Cluster = GetFatCluster(ReadStream);
	if (!Cluster) SET_SOD_MEMORY_MANAGEMENT;
	if (ClusterIndex) {
		*ClusterIndex = Cluster;
	}
	
	FsReadPartition(ReadStream->Partition, Cluster, 1, Buffer);

	if (Copy) {
		memcpy(ReadStream->ReadBuffer, Buffer, ReadStream->RemainingRead);
		ReadStream->ReadBuffer += ReadStream->RemainingRead;
		ReadStream->RemainingRead = 0;

		Buffer = ReadStream->ReadBuffer;
	}

	return Buffer;
}


BOOL FatReadNextFileEntry(struct FAT_DIRECTORY_ENTRY** FileEntry, FAT_READ_STREAM* Stream, wchar_t* FileName, UINT64* FileNameLength) {
	if (!FileEntry || !Stream || !Stream->Set || !FileName) return FALSE;

	if (!*FileEntry) {
		*FileEntry = FatReadNextCluster(Stream, NULL);
		Stream->ThirdBuffer = (void*)*FileEntry;
		if (!*FileEntry) return FALSE;
	}
	if (*(UINT64*)FileEntry >= (UINT64)Stream->ThirdBuffer + Stream->ProtectedClusterSize) {
		*FileEntry = FatReadNextCluster(Stream, NULL);
		if (!*FileEntry) return FALSE;
	}

	struct FAT_DIRECTORY_ENTRY* e = *FileEntry;
	if (!e->allocation_status) return FALSE;
	if ((e->file_attributes & FAT_ATTR_LONG_FILE_NAME) == FAT_ATTR_LONG_FILE_NAME) {
		// Read Long File Name Entry
		if (FAT32ParseLongFileNameEntry((LPVOID)e, &e, NULL, FileName, FileNameLength))
		{
			e++;
			*FileEntry = e;
			return TRUE;
		}
	}
	else if (FAT32ParseShortFileNameEntry(e, FileName, FileNameLength))
	{
		e++;
		*FileEntry = e;
		return TRUE;
	}


	return FALSE;
}

struct FAT_DIRECTORY_ENTRY* FatLastFileEntry(struct FAT_DIRECTORY_ENTRY* FileEntry) {
	if (!FileEntry) return NULL;
	FileEntry--;
	return FileEntry;
}

UINT64 FatGetRemainingRead(FAT_READ_STREAM* ReadStream) {
	if (!ReadStream) return 0;
	return ReadStream->RemainingRead;
}