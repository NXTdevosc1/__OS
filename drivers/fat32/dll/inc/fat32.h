#pragma once


typedef int(__cdecl* FAT32_DRIVER_PRINT) (const char* Format, ...);
typedef void*(__cdecl* FAT32_DRIVER_ALLOCATE_MEMORY) (unsigned long long NumBytes);
typedef void(__cdecl* FAT32_DRIVER_FREE_MEMORY) (void* Heap);


#ifndef __TFS_BOOL
typedef unsigned char __FS_BOOL;
#define __TFS_BOOL
#endif

typedef struct _FAT32_PARTITION FAT32_PARTITION;

typedef __FS_BOOL (__cdecl* FAT32_DRIVER_READ_DRIVE) (void* Buffer, unsigned long long Sector, unsigned long long NumSectors);
typedef __FS_BOOL (__cdecl* FAT32_DRIVER_WRITE_DRIVE) (void* Buffer, unsigned long long Sector, unsigned long long NumSectors);




#pragma pack(push, 1)
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;

#define FAT32_LBA_CHS 0xB
#define FAT32_LBA 0xC

typedef struct _BIOS_PARAMETER_BLOCK {
    UINT16 SectorSize; // In Bytes (Must be 512)
    UINT8 ClusterSize; // Num Sectors in a cluster
    UINT16 ReservedAreaSize; // In our O.S This area contains the BootSector, Boot Manager and FS Descriptors
    UINT8 FatCount; // Usually 2 (We do accept only 2 or 1)
    UINT16 Ignored0; // MaxRootDirEntries (Reserved in FAT32)
    UINT16 Ignored1;
    UINT8 MediaDescriptor;
    UINT16 Ignored2;
} BIOS_PARAMETER_BLOCK;

typedef struct _DOS3_PARAMBLOCK {
    UINT16 SectorsPerTrack;
    UINT16 NumHeads;
    UINT32 HiddenSectors; // Sectors that preceeds the partition, or partition base in LBA
    UINT32 TotalSectors;
} DOS3_PARAMBLOCK;

typedef struct _FAT32_EBIOS_PARAMBLOCK {
    UINT32 SectorsPerFat;
    UINT16 DriveDescription;
    UINT16 Version; // usually set to 0
    UINT32 RootDirStart; // Usually 2
    UINT16 FsInfoLba; // LBA of FS_INFO - HiddenSectors
    UINT16 BootSectorCopyLba; // LBA of BS_COPY - HiddenSectors
    UINT8 Reserved[12];
    UINT8 PhysicalDriveNumber;
    UINT8 Unused;
    UINT8 ExtendedBootSignature; // 0x29
    UINT32 VolumeId;
    UINT8 VolumeLabel[11];
    UINT8 FsLabel[8];
} FAT32_EBIOS_PARAMBLOCK;

typedef enum _FAT_FILE_ATTRIBUTES {
    FAT_READ_ONLY = 1,
    FAT_HIDDEN = 2,
    FAT_SYSTEM = 4,
    FAT_VOLUME_LABEL = 8,
    FAT_DIRECTORY = 0x10,
    FAT_ARCHIVE = 0x20,
    FAT_DEVICE = 0x40
} FAT_FILE_ATTRIBUTES;

// FAT32 VBR

// Used by our os


typedef struct _FAT32_BOOTSECTOR {
    char Jmp[3];
    char OemName[8];
    BIOS_PARAMETER_BLOCK BiosParamBlock;
    DOS3_PARAMBLOCK DosParamBlock;
    FAT32_EBIOS_PARAMBLOCK ExtendedBiosParamBlock;
    UINT8 BootCode[420]; // in our operating system this is not used
    UINT16 BootSignature; // 0xAA55
} FAT32_BOOTSECTOR;

typedef struct _FAT32_FILE_SYSTEM_INFORMATION {
    UINT8 Signature0[4]; // RRaA
    UINT8 Reserved0[480];
    UINT8 Signature1[4]; // rrAa
    UINT32 FreeClusters; // Num free data clusters on volume
    UINT32 RecentlyAllocatedCluster;
    UINT8 Reserved1[12];
    UINT16 NullSignature;
    UINT16 BootSignature; // 0xAA55
    unsigned char Reserved2[510];
    UINT16 BootSignature2;
} FAT32_FILE_SYSTEM_INFORMATION;

typedef struct _FAT32_FILE_ENTRY {
    UINT8 ShortFileName[8]; // Byte0 may contain flags, padded with spaces
    UINT8 ShortFileExtension[3]; // Padded with spaces
    UINT8 FileAttributes;
    UINT8 UserAttributes;
    UINT8 DeletedFileFirstChar;
    struct {
        UINT16 SecondsDivBy2 : 5;
        UINT16 Minutes : 6;
        UINT16 Hours : 5;
    } Time;
    struct {
        UINT16 Day : 5;
        UINT16 Month : 4;
        UINT16 YearFrom1980 : 7;
    } Date;
    UINT8 CreatorUserId; // OS Specific
    UINT8 CreatorGroupId; // OS Specific
    UINT16 FirstClusterHigh;
    struct {
        UINT16 SecondsDivBy2 : 5;
        UINT16 Minutes : 6;
        UINT16 Hours : 5;
    } LastModifiedTime;
    struct {
        UINT16 Day : 5;
        UINT16 Month : 4;
        UINT16 YearFrom1980 : 7;
    } LastModifiedDate;
    UINT16 FirstClusterLow;
    UINT32 FileSize;
} FAT32_FILE_ENTRY;

/*
FOR_LFN : 6 FIRST CHARS + ~ + Number

DRIVER : ~LFN_!EN
*/

#define FAT32_LFN_MARK "~LFN_!EN   "

#define FAT32_ENDOF_CHAIN 0x0FFFFFFF

typedef struct _FAT32_LONG_FILE_NAME_ENTRY {
    UINT8 SequenceNumber; // bit 6 = last logical, 0xE5 = Deleted
    UINT16 Name0[5];
    UINT8 Attributes; // Always 0x0F
    UINT8 Ignored0;
    UINT8 LfnChecksum; // Checksum of dos filename
    UINT16 Name1[6];
    UINT16 Ignored1;
    UINT16 Name2[2];
} FAT32_LONG_FILE_NAME_ENTRY;

#pragma pack(pop)


typedef struct _FAT32_DRIVER_INIT_STRUCTURE {
    __FS_BOOL Debug;
    __FS_BOOL CreateFileRecursive; // if a directory in the path is not found, it will be created

    FAT32_DRIVER_PRINT Print; // Used if Debug == 1
    FAT32_DRIVER_ALLOCATE_MEMORY AllocateMemory;
    FAT32_DRIVER_FREE_MEMORY FreeMemory;
    FAT32_DRIVER_READ_DRIVE Read;
    FAT32_DRIVER_WRITE_DRIVE Write;
} FAT32_DRIVER_INIT_STRUCTURE;

typedef struct _FAT32_PARTITION {
    FAT32_BOOTSECTOR BootSector;
    FAT32_FILE_SYSTEM_INFORMATION FileSystemInformation;
    UINT32 SecondaryFsInfo;
    UINT32 LastReadClusterSector;
    UINT64 RootDirLba;
    char GeneralPurposeSector[0x200];
    char SecondaryPurposeSector[0x200];
    UINT32 FatSector[0x80];
    void* ReadCluster;
    UINT32 FatSectorNumber;
} FAT32_PARTITION, *PFAT32_PARTITION;

#ifndef __DLL_EXPORTS
#define FSBASEAPI __declspec(dllimport) __cdecl
#else
#define FSBASEAPI __declspec(dllexport) __cdecl
#endif

typedef enum _FAT_STATUS{
    FAT_SUCCESS = 0,
    FAT_NO_ENOUGH_SPACE = 1,
    FAT_READ_ERROR = 2,
    FAT_WRITE_ERROR = 3,
    FAT_NOT_FOUND = 4,
    FAT_INVALID_PARAMETER = 5,
    FAT_BAD_CLUSTER = 6,
    FAT_CONTINUE = 7,
    FAT_END = 8
} FAT_STATUS;

typedef enum _FILE_EDIT_PARAMETERS {
    FILE_EDIT_ATTRIBUTES = 1,
    FILE_EDIT_CLUSTER = 2,
    FILE_EDIT_FILE_SIZE = 4
} FILE_EDIT_PARAMETERS;

#define FAT_MAX_PATH 0x200

typedef struct _FAT_FILE_INFORMATION {
    UINT32 ClusterStart;
    UINT16 FileName[FAT_MAX_PATH];
    UINT32 FatAttributes;
    UINT32 CreationTimeAndDate;
    UINT32 ModificationTimeAndDate;
    UINT32 ParentDirectoryCluster; // Cluster Start of parent dir
    UINT32 FileSize;
} FAT_FILE_INFORMATION;

// Should be called once for every dll
__FS_BOOL FSBASEAPI Fat32DriverInit(FAT32_DRIVER_INIT_STRUCTURE* InitStructure);


/*
FAT32_CREATE_PARTITION :
Creates partition with DLL OEM_NAME, 
*/


PFAT32_PARTITION FSBASEAPI Fat32CreatePartition(UINT32 BaseLba, UINT32 NumClusters, UINT8 ClusterSize, UINT8* VolumeLabel, UINT16 NumReservedSectors, char* BootSector);
__FS_BOOL FSBASEAPI Fat32WriteReservedArea(PFAT32_PARTITION Partition, UINT16 Sector, UINT16 NumSectors, void* Buffer);
int FSBASEAPI Fat32CreateFile(PFAT32_PARTITION Partition, UINT16* Path, FAT_FILE_ATTRIBUTES FileAttributes);

__FS_BOOL FSBASEAPI Fat32EditFile(PFAT32_PARTITION Partition, UINT16* Path, FILE_EDIT_PARAMETERS FileEditParameters, FAT_FILE_ATTRIBUTES NewAttributes, UINT32 ClusterFileSize, UINT16 NewModificationDate);
__FS_BOOL FSBASEAPI Fat32GetFileInformation(PFAT32_PARTITION Partition, UINT16* FilePath, FAT_FILE_INFORMATION* FileInformation);

UINT32 FSBASEAPI Fat32GetLastCluster(PFAT32_PARTITION Partition, UINT32 ClusterChainStart);

UINT32 FSBASEAPI Fat32GetNextCluster(PFAT32_PARTITION Partition, UINT32 LastCluster);

int FSBASEAPI Fat32WriteFile(PFAT32_PARTITION Partition, UINT16* Path, void* _Buffer, UINT32 Position, UINT32 NumBytes);

#ifdef __DLL_EXPORTS

FAT32_FILE_ENTRY* Fat32FindDirectoryClusterEntry(PFAT32_PARTITION Partition, UINT16* FileName, void* ClusterBuffer, UINT16* EntryName);



void Fat32MemCopy(void* Destination, const void* Source, unsigned long long NumBytes);
void Fat32MemClear(void* Destination, unsigned long long NumBytes);

// Return VALUE : First Cluster, 0xFFFFFFFF if the function fails
UINT32 Fat32AllocateClusterChain(PFAT32_PARTITION Partition, UINT32 ClusterStart, UINT32 NumClusters);

__FS_BOOL Fat32MarkBadClusterChain(PFAT32_PARTITION Partition, UINT32 ClusterStart);
__FS_BOOL Fat32UnmarkBadClusterChain(PFAT32_PARTITION Partition, UINT32 ClusterStart);

__FS_BOOL Fat32FreeClusterChain(PFAT32_PARTITION Partition, UINT32 ClusterStart);
UINT32 Fat32RetreiveClusterCount(PFAT32_PARTITION Partition, UINT32 ClusterStart);

// return value : 0 on error, 1 = continue, 2 = last entry reached
int Fat32ParsePath(UINT16* Path, UINT16** AdvancePath, UINT16* Entry /*stored in lowercase*/);
__FS_BOOL Fat32FindEntry(PFAT32_PARTITION Partition, UINT16* FileName, UINT32 Cluster, FAT32_FILE_ENTRY* DestEntry);
int Fat32AllocateEntry(PFAT32_PARTITION Partition, UINT32 Cluster, UINT16* FileName, FAT_FILE_ATTRIBUTES FileAttributes);
int Fat32DeleteEntry(PFAT32_PARTITION Partition, UINT16* FileName, UINT32 Cluster);




UINT16 Fat32StrlenA(const UINT8* str);
UINT16 Fat32StrlenW(const UINT16* str);


/*
READ CONTIGUOUS Clusters with Fat32ReadCluster
First read with ClusterStart (save return value)
while(Fat32ReadCluster(...) == FAT_CONTINUE) ...
*/
int Fat32ReadCluster(PFAT32_PARTITION Partition, UINT32* _Cluster, void* Buffer, int* LastCode /*set to 0*/);
__FS_BOOL Fat32WriteCluster(PFAT32_PARTITION Partition, UINT32 Cluster, void* Buffer);


extern FAT32_DRIVER_INIT_STRUCTURE StartupInfo;
#define FAT32_ALIGN_CHECK(x, Align) (x & (Align - 1) == 0)

#endif