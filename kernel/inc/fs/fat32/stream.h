#pragma once
#include <dsk/disk.h>
#include <krnltypes.h>
#include <fs/fat32/fat32.h>
typedef struct _FAT_READ_STREAM{
	BOOL Set;
	UINT32 InitialCluster;
	BOOL InitialRead; // set to false to read the initial cluster first
	UINT32* FatBuffer; // Buffer of current cluster
	char* ReadBuffer;
	void* SecondBuffer;
	void* ThirdBuffer; // for e.g. Used in ReadNextFileEntry

	UINT32 FatIndex; // Fat index in cluster
	UINT32 MaxIndex;
	UINT32 FatCluster;
	UINT32 ReadSize; // Max Read Size
	UINT32 RemainingRead;
	UINT32 ClusterSize; // In bytes
	UINT32 ProtectedClusterSize; // In bytes

	PARTITION_INSTANCE* Partition;
} FAT_READ_STREAM;

FAT_READ_STREAM* FatOpenReadStream(UINT32 InitialCluster, UINT32 ReadSize, void* ReadBuffer, PARTITION_INSTANCE* Partition);

BOOL FatCloseReadStream(FAT_READ_STREAM* ReadStream);

BOOL FatReadNextFileEntry(struct FAT_DIRECTORY_ENTRY** FileEntry, FAT_READ_STREAM* Stream, wchar_t* FileName, UINT64* FileNameLength);

struct FAT_DIRECTORY_ENTRY* FatLastFileEntry(struct FAT_DIRECTORY_ENTRY* FileEntry);

void* FatReadNextCluster(FAT_READ_STREAM* ReadStream, UINT32* ClusterIndex);

UINT64 FatGetRemainingRead(FAT_READ_STREAM* ReadStream);