#include <dsk/disk.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <kernel.h>
OPEN_FILE_LIST GeneralFileList = { 0 };
DISK_DEVICE_LIST GeneralDiskDeviceList = { 0 };
PARTITION_LIST GeneralPartitionList = { 0 };


PARTITION_INSTANCE* MainPartition = NULL;
PARTITION_INSTANCE* SystemPartition = NULL;
#include <stdlib.h>
#include <math.h>
DISK_DEVICE_INSTANCE* KERNELAPI CreateDisk(UINT64* DiskId, UINT64 SecurityDescriptor, DISK_INFO* DiskInformation, RFDEVICE_OBJECT Device) {
	if (!DiskInformation || !ValidateDevice(Device)) return NULL;

	DISK_DEVICE_LIST* DiskDeviceList = &GeneralDiskDeviceList;

	for (UINT64 ListIndex = 0;;ListIndex++) {
		for (UINT64 i = 0; i < UNITS_PER_LIST; i++) {
			if (!DiskDeviceList->Disks[i].Present) {
				DISK_DEVICE_INSTANCE* Disk = &DiskDeviceList->Disks[i];
				
				

				if (Disk->Present) continue;
				Disk->Present = TRUE;
				Disk->Device = Device;
				Disk->DiskId = ListIndex * UNITS_PER_LIST + i;
				memcpy(&Disk->DiskInformation, DiskInformation, sizeof(DISK_INFO));
				Disk->SecurityDescriptor = SecurityDescriptor;
				

				if (DiskId) *DiskId = Disk->DiskId;

				Device->ExtensionPointer = Disk;

				return Disk;
			}
		}
		if (!DiskDeviceList->Next) {
			DiskDeviceList->Next = AllocatePool(sizeof(DISK_DEVICE_LIST));
			if (!DiskDeviceList->Next) SET_SOD_MEMORY_MANAGEMENT;
			SZeroMemory(DiskDeviceList->Next);
		}
		DiskDeviceList = DiskDeviceList->Next;
	}
}

BOOL KERNELAPI SetPartitionName(PARTITION_INSTANCE* Partition, LPWSTR PartitionName) {
	if (!ValidatePartition(Partition)) return FALSE;
	if (!PartitionName) {
		Partition->PartitionName = NULL;
		return TRUE;
	}

	UINT64 len = wstrlen(PartitionName);
	if (!len) return FALSE;
	LPWSTR Copy = AllocatePool((len + 1) << 1); // len * 2 + 2
	if (!Copy) SET_SOD_MEMORY_MANAGEMENT;
	memcpy16(Copy, PartitionName, len);
	Copy[len] = 0;
	Partition->PartitionName = Copy;

	
	return TRUE;
}

BOOL KERNELAPI ValidateDisk(DISK_DEVICE_INSTANCE* Disk) {
	if (!Disk) return FALSE;
	UINT64 NumLists = Disk->DiskId / UNITS_PER_LIST;
	UINT64 Index = Disk->DiskId % UNITS_PER_LIST;
	DISK_DEVICE_LIST* List = &GeneralDiskDeviceList;
	for (UINT64 i = 0; i < Index; i++) {
		if (!List->Next) return FALSE;
		List = List->Next;
	}

	if (&List->Disks[Index] == Disk) return TRUE;

	return FALSE;
}

PARTITION_INSTANCE* KERNELAPI CreatePartition(LPWSTR PartitionName, UINT64* PartitionId, UINT64 SecurityDescriptor, DISK_DEVICE_INSTANCE* DiskDevice, DISK_INFO* PartitionInformation, LPVOID FsInfo, PARTITION_MANAGEMENT_INTERFACE* ManagementInterface) {
	if (!DiskDevice || !PartitionInformation) return NULL;
	
	PARTITION_LIST* PartitionList = &GeneralPartitionList;
	for (UINT64 ListIndex = 0;;ListIndex++) {
		for (UINT64 i = 0; i < UNITS_PER_LIST; i++) {
			if (!PartitionList->Partitions[i].Present) {
				PARTITION_INSTANCE* Partition = &PartitionList->Partitions[i];
				


				if (Partition->Present) continue; // Check if partition is controlled by another thread
				Partition->Present = TRUE;
				Partition->PartitionId = ListIndex * UNITS_PER_LIST + i;
				if (PartitionId) *PartitionId = Partition->PartitionId;
				Partition->SecurityDescriptor = SecurityDescriptor;

				if (SecurityDescriptor & DISK_SYSTEM) SystemPartition = Partition;

				if (SecurityDescriptor & DISK_MAIN) MainPartition = Partition;

				memcpy(&Partition->PartitionInformation, PartitionInformation, sizeof(DISK_INFO));
				Partition->Disk = DiskDevice;
				
				Partition->FsInfo = FsInfo;
				if (ManagementInterface) {
					memcpy(&Partition->Interface, ManagementInterface, sizeof(PARTITION_MANAGEMENT_INTERFACE));
				}

				SetPartitionName(Partition, PartitionName);
				return Partition;
			}
		}
		if (!PartitionList->Next) {
			PartitionList->Next = AllocatePool(sizeof(PARTITION_LIST));
			if (!PartitionList->Next) SET_SOD_MEMORY_MANAGEMENT;
			SZeroMemory(PartitionList->Next);
		}

		PartitionList = PartitionList->Next;
	}
}

PARTITION_INSTANCE* KERNELAPI GetPartition(UINT64 PartitionId) {
	UINT64 ListIndex = PartitionId / UNITS_PER_LIST;
	UINT64 Index = PartitionId % UNITS_PER_LIST;

	PARTITION_LIST* List = &GeneralPartitionList;
	for (UINT64 i = 0; i < ListIndex; i++) {
		if (!List->Next) return NULL;
		List = List->Next;
	}
	PARTITION_INSTANCE* Partition = &List->Partitions[Index];
	if (!Partition->Present) return NULL;

	return Partition;
}

BOOL KERNELAPI ValidatePartition(PARTITION_INSTANCE* Partition) {
	if (!Partition) return FALSE;
	if (GetPartition(Partition->PartitionId) == Partition) return TRUE;
	
	return FALSE;
}

BOOL KERNELAPI DestroyPartition(PARTITION_INSTANCE* Partition) {
	if (!ValidatePartition(Partition)) return FALSE;
	RemoteFreePool(kproc, Partition->PartitionName);
	if (Partition == MainPartition) MainPartition = NULL;
	if (Partition == SystemPartition) SystemPartition = NULL;
	SZeroMemory(Partition);
	return TRUE;
}

BOOL KERNELAPI RemoveDisk(DISK_DEVICE_INSTANCE* Disk) {
	if (!ValidateDisk(Disk)) return FALSE;
	SZeroMemory(Disk);
	return TRUE;
}

OPEN_FILE_LIST* KERNELAPI CreateFileTable() {
	OPEN_FILE_LIST* ret = AllocatePool(sizeof(OPEN_FILE_LIST));
	if (!ret) SET_SOD_MEMORY_MANAGEMENT;
	SZeroMemory(ret);
	return ret;
}

FILE KERNELAPI AllocateFile(OPEN_FILE_LIST* FileList) {
	if (!FileList) return NULL;
	OPEN_FILE_LIST* InitialList = FileList;
	for (UINT64 ListIndex = 0;; ListIndex++) {
		for (UINT64 i = 0; i < UNITS_PER_LIST; i++) {
			if (!FileList->Files[i].Open) {
				FILE file = &FileList->Files[i];
				


				if (file->Open) continue;
				SZeroMemory(file);
				file->Open = TRUE;
				file->FileId = ListIndex * UNITS_PER_LIST + i;
				file->FileList = InitialList;
				return file;
			}
		}
		if (FileList->Next) {
			FileList->Next = AllocatePool(sizeof(OPEN_FILE_LIST));
			if (!FileList->Next) SET_SOD_MEMORY_MANAGEMENT;
			SZeroMemory(FileList->Next);
		}
		FileList = FileList->Next;
	}
}

BOOL KERNELAPI ReleaseFile(FILE File) {
	if (!File) return FALSE;
	CloseHandle(File->Handle);
	RemoteFreePool(kproc, File->Path);
	SZeroMemory(File);
	return TRUE;
}

PARTITION_INSTANCE* KERNELAPI ResolvePartition(LPWSTR Path, LPWSTR* InPartitionPath) {
	// memcmp adding 2 to length to include 0 '\0'
	if (!Path || !InPartitionPath) return NULL;
	if (memcmp(Path, L"//", 4)) {
		*InPartitionPath = Path + 2;
		return GetMainPartition(NULL);
	}
	else if (memcmp(Path, L"///", 6)) {
		*InPartitionPath = Path + 3;
		return GetSystemPartition(NULL);
	}

	UINT64 PartitionIndexerLength = 0;
	BOOL PathVerified = FALSE;
	LPWSTR tmp = Path;
	while (*tmp) {
		if (memcmp(tmp, L":/", 4)) {
			tmp += 2;
			*InPartitionPath = tmp;
			if (PartitionIndexerLength) PathVerified = TRUE;
			break;
		}
		PartitionIndexerLength++;
		tmp++;
	}
	if (!PathVerified) return NULL;
	PathVerified = FALSE;
	// Verify first character of the path must be a letter
	UINT64 PartitionId = 0;
	if (*Path >= L'a' && *Path <= L'z') {
		PartitionId = * Path - L'a';
		Path++;
		PartitionIndexerLength--;
		PathVerified = TRUE;
	}
	else if (*Path >= L'A' && *Path <= L'Z') {
		PartitionId = *Path - L'A';
		Path++;
		PartitionIndexerLength--;
		PathVerified = TRUE;
	}

	if (PartitionIndexerLength) {
		// Resolve Index Number for e.g. a123:/
		UINT64 maxchar = PartitionIndexerLength - 1;
		UINT16 c = 0;
		for (INT64 i = maxchar; i >= 0; i--, c++) {
			if (c) {
				UINT64 power = powi(10 * Path[i], c);
				PartitionId += power;
			}
			else PartitionId += i;
		}
	}
	PARTITION_INSTANCE* Partition = GetPartition(PartitionId); // if partition does not exist, return is NULL
	return Partition;
}

PARTITION_INSTANCE* KERNELAPI GetMainPartition(UINT64* PartitionId) {
	if (MainPartition && PartitionId) *PartitionId = MainPartition->PartitionId;
	return MainPartition;
}
PARTITION_INSTANCE* KERNELAPI GetSystemPartition(UINT64* PartitionId) {
	if (SystemPartition && PartitionId) *PartitionId = SystemPartition->PartitionId;
	return SystemPartition;
}

LPWSTR KERNELAPI ResolveNextFileName(LPWSTR Path, LPWSTR FileName, UINT64* FileNameLength) // Return value is the path pushed
{
	if (!Path || !FileName || !*Path) return NULL;
	UINT64 NumChars = 0;
	while (*Path) {
		if (*Path == L'/') {
			Path++;
			if (FileNameLength) *FileNameLength = NumChars;
			break;
		}
		*FileName = *Path;
		FileName++;
		NumChars++;
		Path++;
	}
	*FileName = 0;
	if (FileNameLength) *FileNameLength = NumChars;
	return Path;
}

BOOL KERNELAPI ValidateFile(FILE File) {
	if (!File || !File->Open || !File->FileList) return FALSE;
	UINT64 ListIndex = File->FileId / UNITS_PER_LIST;
	UINT64 Index = File->FileId % UNITS_PER_LIST;
	OPEN_FILE_LIST* list = File->FileList;
	for(UINT64 i = 0;i<ListIndex;i++){
		if (!list->Next) return FALSE;
		list = list->Next;
	}
	if (&list->Files[Index] == File) return TRUE;
	return FALSE;
}

BOOL KERNELAPI SetMainPartition(PARTITION_INSTANCE* Partition){
	if (!ValidatePartition(Partition)) return FALSE;
	MainPartition = Partition;
	return TRUE;
}
BOOL KERNELAPI SetSystemPartition(PARTITION_INSTANCE* Partition) {
	if(!ValidatePartition(Partition)) return FALSE;
	SystemPartition = Partition;
	return TRUE;
}

BOOL KERNELAPI FixFilePath(LPWSTR Path) {
	if (!Path || !*Path) return FALSE;
	UINT32 len = wstrlen(Path);
	UINT16 Copy[MAX_FILE_NAME] = { 0 };
	UINT16* LpCopy = Copy;
	memcpy16(Copy, Path, len);
	Copy[len] = 0;
	UINT32 ResultLength = 0;
	BOOL SeparatorFound = TRUE;
	while(*LpCopy) {
		if (SeparatorFound) {
			while (*LpCopy == L'/' || *LpCopy == L'\\') {
				LpCopy++;
			}
			SeparatorFound = FALSE;
		}
		else {
		if (*LpCopy == L'/' || *LpCopy == L'\\') {
			SeparatorFound = TRUE;
			*LpCopy = L'/';
			if (!(*(LpCopy + 1))) {
				break;
			}
		}
		Path[ResultLength] = *LpCopy;
		LpCopy++;
		ResultLength++;
		}
	}
	
	Path[ResultLength] = 0;
	return TRUE;
}