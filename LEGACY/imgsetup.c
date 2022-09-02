
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fat32.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>

#define IMAGE_NAME "os.img"

#define BOOT_SECT "x86_64/bootsect.bin"

#define PARTITION_BOOT_SECTOR "x86_64/partbootsect.bin"

#define BOOT_STAGE2 "x86_64/osbootmgr.bin"
#define ORIGINAL_PARTITION "format.img"



#define KERNEL_PATH L"OS\\System\\OSKRNLX64.exe"

#define ERROR_EXIT(Str) {printf("%s\nLAST_ERROR : %d\n", Str, GetLastError()); system("pause");exit(-1);}

#define BOOT_MAJOR 1
#define BOOT_MINOR 0

/*

SCHEME : USING (GPT PARTITIONNING)

Sector 0 - BOOT_SECTOR
Sector 1 - BOOT_MGR


*/

#pragma comment(lib, "legacy_stdio_definitions.lib")

#pragma pack(push, 1)


struct BIOS_PARAMETER_BLOCK{
    UINT16 sector_size;
    UINT8 cluster_size;
    UINT16 reserved_area_size;
    UINT8 fat_count;
    UINT16 max_root_dir_entries; // not set in fat32, just for fat12, fat16
    UINT16 total_sectors; // not set in fat32
    UINT8 media_descriptor;
    UINT16 sectors_per_fat; // not set in fat32
};

struct DOS3_PARAMETER_BLOCK{
    struct BIOS_PARAMETER_BLOCK dos2bpb;
    UINT16 sectors_per_track;
    UINT16 num_heads;
    UINT32 hidden_sectors; //Count of hidden sectors preceding the partition that contains this FAT volume.
    UINT32 total_sectors;
};
struct FAT32_EXTENDED_BIOS_PARAMETER_BLOCK{
    struct DOS3_PARAMETER_BLOCK dos_bpb;
    UINT32 sectors_per_fat;
    UINT16 drive_description;
    UINT16 version;
    UINT32 root_dir_start;
    UINT16 fs_info_lba;
    UINT16 fat32_bscopy; //First logical sector number of a copy of the three FAT32 boot sectors, typically 6.
    UINT8 reserved[12];
    UINT8 physical_drive_number;
    UINT8 unused; // for FAT12/FAT16 (Used for various purposes; see FAT12/FAT16)
    UINT8 extended_boot_signature; // 0x29
    UINT32 volume_id;
    UINT8 volume_label[11];
    UINT8 fs_label[8];
};


struct FAT32_FSINFO{
    UINT32 fsinfo_sig0; // RRaA
    UINT8 reserved0[480];
    UINT32 fsinfo_sig1; // rrAa
    UINT32 free_clusters; // last known number of free data clusters on the volume
    UINT32 recently_allocated_cluster;
    UINT8 reserved1[12];
    UINT16 signull;
    UINT16 signature; // 0x55AA
};

struct FAT32_BOOTSECTOR {
    UINT8 jmp[3];
    UINT8 oem_name[8];
    struct FAT32_EXTENDED_BIOS_PARAMETER_BLOCK fatpb;
    UINT8 boot_code[420];
    UINT16 boot_signature;
};


enum MBR_PARTITION_TYPE{
    MBR_FAT32 = 0x0C,
    MBR_NTFS = 0x07,
    MBR_GPT = 0xEE
};


#define MBR_STATUS_ACTIVE 0x80
#define BOOTMGR_SIZE 0x20000
#define NUM_GPT_PARTITIONS 0x80
#define RESERVED_AREA_SIZE (BOOTMGR_SIZE + 0x20000)

typedef struct _CHS_ADDRESS {
    UINT8 Head;
    UINT8 Sector;
    UINT8 Cylinder;
} CHS_ADDRESS;

struct MBR_PARTITION_ENTRY{
    UINT8 Status; // 0x80 [bit 7] means active (or bootable)
    CHS_ADDRESS StartingChsAddress;
    UINT8 PartitionType;
    CHS_ADDRESS EndingChsAddress; // HEAD, SECTOR, CYLINDER
    DWORD LbaStart;
    DWORD NumSectors;
};



#define BOOT_CODE_LENGTH 440
#define BOOT_HEADER_LENGTH (0x400 + (0x200 * 30))




typedef struct _MBR_HDR{
    UINT8 BootCode[BOOT_CODE_LENGTH]; // Actually 440
    DWORD UniqueDiskId;
    UINT16 Reserved;
    struct MBR_PARTITION_ENTRY Partitions[4];
    UINT16 BootSignature;
} MASTER_BOOT_RECORD;

char GPT_SIGNATURE[8] = "EFI PART";
#define GPT_REVISION 0x100
typedef struct _GUID_PARTITION_TABLE {
    UINT8 Signature[8];
    DWORD Revision; // 1.0 for UEFI 2.8
    DWORD HeaderSize; // 0x5C
    DWORD CRC32;
    DWORD Reserved0;
    UINT64 CurrentLba;
    UINT64 BackupLba;
    UINT64 FirstUsablePartitionLba;
    UINT64 LastUsablePartitionLba;
    GUID DiskGuid;
    UINT64 PartitionArrayStartingLba; // Set to 2
    DWORD NumPartitionEntries;
    DWORD PartitionEntrySize; // Usually 0x80
    DWORD CRC32_1;
    // DWORD Reserved[420];
} GUID_PARTITION_TABLE;

typedef struct _GUID_PARTITION_ENTRY {
    GUID PartitionTypeGuid;
    GUID UniquePartitionGuid;
    UINT64 FirstLba;
    UINT64 LastLba;
    UINT64 AttributeFlags;
    UINT16 PartitionName[36];
} GUID_PARTITION_ENTRY;


UINT8 BDP_GUID[16] = {0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7};
#define BASIC_DATA_PARTITION_GUID *(GUID*)BDP_GUID;

typedef struct _BOOT_POINTER_TABLE_ENTRY {
    UINT64 ClusterStart;
    UINT32 NumClusters;
    UINT16 PathLength;
    UINT16 Path[0x100];
} BOOT_POINTER_TABLE_ENTRY;

typedef struct _BOOT_POINTER_TABLE {
    UINT32 Magic; // 0xEEF7C8D9
    UINT8 StrMagic[4]; // 'PRTP'
    // Currently set to 1.0
    UINT16 BootLoaderMajorVersion;
    UINT16 BootLoaderMinorVersion;
    UINT32 EntrySize;
    UINT32 NumEntries;
    UINT32 OffsetToEntries;
    // MAJOR Kernel Entry
    UINT64 KernelClusterStart;
    UINT32 KernelFileNumClusters;
    UINT64 AllocationTableLba;
    UINT64 DataClustersLba;
    BOOT_POINTER_TABLE_ENTRY Entries[];
} BOOT_POINTER_TABLE;

#pragma pack(pop)

#define EXTENDED_BOOT_AREA_SIZE (RESERVED_AREA_SIZE >> 9) // In Sectors
#define EXTENDED_FS_AREA_OFFSET 0xE0

typedef struct _IMAGE_STRUCTURE {
    MASTER_BOOT_RECORD Mbr;
    // GPT HEADER SECTOR
    GUID_PARTITION_TABLE Gpt;
    UINT8 Reserved0[420];
    // GPT ENTRIES SECTOR
    GUID_PARTITION_ENTRY Partitions[NUM_GPT_PARTITIONS];
    
    char BootManagerCode[BOOTMGR_SIZE];
} IMAGE_STRUCTURE;




// Only path is set, other details are set by SetupFs Function
#define NUM_BOOT_POINTER_ENTRIES 9
UINT16* BootPointerPaths[NUM_BOOT_POINTER_ENTRIES] = {
    L"OS\\System\\KeConfig\\$BOOTCONFIG",
    L"OS\\System\\KeConfig\\$DRVTBL",
    L"OS\\System\\osdll.dll",
    L"OS\\System\\ddk.dll",
    L"OS\\System\\ehci.sys",
    L"OS\\System\\ahci.sys",
    L"OS\\Fonts\\zap-light16.psf",
    L"OS\\System\\xhci.sys",
    L"OS\\Fonts\\segoeui.ttf"
};

__declspec(align(0x1000)) IMAGE_STRUCTURE Image = {0};
__declspec(align(0x1000)) char Sector[0x200] = {0};


HANDLE Drive = INVALID_HANDLE_VALUE;

#define IMAGE_START_LBA 40

UINT16 StrlenW(UINT16* Str) {
    register UINT16 Len = 0;
    while(*Str++) Len++;
    return Len;
}

int SetupLocalFs(int argc, char** argv, HANDLE OsImage);

int main(int argc, char** argv) {

    if(argc == 1) {
        argc = 3;
        argv[1] = "devctl";
        argv[2] = "D";
    }
    
    if(argc < 2) {
        printf("INVALID_ARGS\n");
        system("pause");
        return -1;
    }
    if(strcmp(argv[1], "fstest") == 0) {
        printf("- FSTEST:\n");
        HANDLE OsImage = CreateFileA(IMAGE_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if(OsImage == INVALID_HANDLE_VALUE) {
            printf("Failed to create os image.");
            return -1;
        }
        Drive = OsImage;
        
        SetupLocalFs(argc, argv, OsImage);
        CloseHandle(OsImage);
    }else if(strcmp(argv[1], "boot") == 0) {

    }else if(strcmp(argv[1], "devctl") == 0) {
        printf("- DEVCTL (%s) \n", argv[2]);
        if(argc < 3) {
            printf("INVALID_ARGS\n");
            system("pause");
            return -1;
        }
        HANDLE Volume = INVALID_HANDLE_VALUE;

        char VolumeName[100] = {0};

        DWORD DiskBitmask = GetLogicalDrives();

        UINT LogicalDriveNumber = argv[2][0] - 'A';

        printf("TARGET_DRIVE_NUMBER : %d\n", LogicalDriveNumber);

    
        if(!(DiskBitmask & (1 << LogicalDriveNumber))) ERROR_EXIT("NO_FOUND_DRIVES");

        UINT PhysicalDriveNumber = 0xFFFFFFFF;
        VOLUME_DISK_EXTENTS DiskExtents = {0};

        DWORD BytesReturned = 0;


        sprintf_s(VolumeName, 100, "\\\\.\\%c:", argv[2][0]);
        Volume = CreateFileA(VolumeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
        HANDLE Partition = Volume;
        if(Volume == INVALID_HANDLE_VALUE) ERROR_EXIT("FAILED_TO_OPEN_DRIVE");
        if(!DeviceIoControl(
            Volume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
            NULL, 0, &DiskExtents, sizeof(VOLUME_DISK_EXTENTS), &BytesReturned, NULL
        )) {
            
            ERROR_EXIT("DEVICE_IOCTL Failed.");
        }

        if(!DeviceIoControl(Volume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &BytesReturned, NULL)) ERROR_EXIT("Failed to lock the volume.");


        PhysicalDriveNumber = DiskExtents.Extents[0].DiskNumber;
            
        printf("Found Physical Drive %d\n", PhysicalDriveNumber);

        sprintf_s(VolumeName, 100, "\\\\.\\PhysicalDrive%d", PhysicalDriveNumber);

        printf("Volume Name : %s\n", VolumeName);
        // CloseHandle(Volume);
        Volume = CreateFileA(VolumeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
        if(Volume == INVALID_HANDLE_VALUE) 
            ERROR_EXIT("OPEN_VOLUME FAILURE");


        OVERLAPPED Overlapped = {0};

        Overlapped.Offset = 40 << 9;
        if(!ReadFile(Volume, Sector, 0x200, NULL, &Overlapped)) {
            ERROR_EXIT("Failed to read from hard drive.");
        }
        printf("Volume Buffer : %s\n", Sector);
        Drive = Volume;
        SetupLocalFs(argc, argv, Volume);
        if(!DeviceIoControl(Partition, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &BytesReturned, NULL)) ERROR_EXIT("Failed to dismount the volume");
        if(!DeviceIoControl(Partition, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &BytesReturned, NULL)) ERROR_EXIT("Failed to unlock the volume");
        CloseHandle(Partition);
        CloseHandle(Volume);

        return 0;        
    }else {
        printf("INVALID_ARGS\n");
        system("pause");
        return -1;
    }

    
}


int DriverPrint(const char* Format, ...) {
    return printf(Format); // parameter are on the stack (no need to copy them)
}

__FS_BOOL DriverReadDrive(void* Buffer, unsigned long long Sector, unsigned long long NumSectors) {
    DWORD DistanceHigh = ( Sector << 9 ) >> 32;
    
    SetFilePointer(Drive, Sector << 9, &DistanceHigh, FILE_BEGIN);
    return ReadFile(Drive, Buffer, NumSectors << 9, NULL, NULL);
}

__FS_BOOL DriverWriteDrive(void* Buffer, unsigned long long Sector, unsigned long long NumSectors) {
    DWORD DistanceHigh = ( Sector << 9 ) >> 32;
    SetFilePointer(Drive, Sector << 9, &DistanceHigh, FILE_BEGIN);
    return WriteFile(Drive, Buffer, NumSectors << 9, NULL, NULL);
}

void* malloc(size_t size);
void free(void* Heap);

#define LEN_ROOT_PATH (sizeof("../kernel/iso/") - 1)

FAT32_PARTITION* Partition = NULL;

UINT16 FoundFilePath[MAX_PATH] = {0};
UINT16 PhysicalIndirFilePath[MAX_PATH] = {0};
void CopyDirectoryFiles(UINT16* ParentDirName, UINT16* DirName) {
    WIN32_FIND_DATAW* FindData = malloc(sizeof(WIN32_FIND_DATAW));
    UINT16* szdir = malloc(MAX_PATH << 1);

    

    UINT16 LenParentDir = StrlenW(ParentDirName);
    UINT16 LenDir = StrlenW(DirName);

    memcpy(szdir, ParentDirName, LenParentDir << 1);
    szdir[LenParentDir] = '/';
    
    memcpy(&szdir[LenParentDir + 1], DirName, LenDir << 1);
    szdir[LenParentDir + LenDir + 1] = '/';
    szdir[LenParentDir + LenDir + 2] = '*';
    szdir[LenParentDir + LenDir + 3] = 0;

    

    HANDLE hFindFile = FindFirstFileW(szdir, FindData);

    if(hFindFile == INVALID_HANDLE_VALUE) ERROR_EXIT("FAILED_FIND_FIRST_FILE");

    szdir[LenParentDir + LenDir + 1] = 0; // Remove last indicators
    UINT16 LenLogicalSzdir = StrlenW(szdir) - LEN_ROOT_PATH;
    while(FindNextFileW(hFindFile, FindData)) {
        // do not count . & .. directories
        if(!memcmp(FindData->cFileName, L".", 4) || !memcmp(FindData->cFileName, L"..", 6)) continue;
        // Copying logical path
        memcpy(FoundFilePath, &szdir[LEN_ROOT_PATH], LenLogicalSzdir << 1);
        FoundFilePath[LenLogicalSzdir] = L'/';
        UINT16 LenFileName = StrlenW(FindData->cFileName);
        memcpy(&FoundFilePath[LenLogicalSzdir + 1], FindData->cFileName, LenFileName << 1);
        FoundFilePath[LenLogicalSzdir + 1 + LenFileName] = 0;
        
        // printf("Path : %ls - File : %ls, Attributes : %I32x\n", FoundFilePath, FindData->cFileName, FindData->dwFileAttributes);
        FAT_FILE_ATTRIBUTES Attributes = 0;
        if(FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) Attributes |= FAT_DIRECTORY;
        if(Fat32CreateFile(Partition, FoundFilePath, Attributes) != FAT_SUCCESS) ERROR_EXIT("FAT32 : Failed to Create IMAGE_FILE.");
        
        if(FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            CopyDirectoryFiles(szdir, FindData->cFileName);
        }else {
            // Copy file content
            StringCchPrintfW(PhysicalIndirFilePath, MAX_PATH, L"%ls/%ls", szdir, FindData->cFileName);
            printf("FILE_PATH : %ls\n", PhysicalIndirFilePath);
            HANDLE File = CreateFileW(PhysicalIndirFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0 , NULL);
            if(File == INVALID_HANDLE_VALUE) ERROR_EXIT("Failed to Open IMAGE_FILE.");
            UINT32 FileSize = GetFileSize(File, NULL);
            if(!FileSize) {
                CloseHandle(File);
                continue;
            }
            void* Buffer = malloc(FileSize);
            if(!ReadFile(File, Buffer, FileSize, NULL, NULL)) ERROR_EXIT("Failed to Read IMAGE_FILE.");
            if(Fat32WriteFile(Partition, FoundFilePath, Buffer, 0, FileSize) != FAT_SUCCESS) ERROR_EXIT("FAT32 : Failed to write IMAGE_FILE.");
            free(Buffer);
            CloseHandle(File);
            
        }
    }


    FindClose(hFindFile);
}

int SetupLocalFs(int argc, char** argv, HANDLE OsImage){
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    


    
    FAT32_DRIVER_INIT_STRUCTURE Fat32Startup = {0};
    Fat32Startup.Debug = 1;
    Fat32Startup.AllocateMemory = malloc;
    Fat32Startup.FreeMemory = free;
    Fat32Startup.Print = printf;
    Fat32Startup.Read = DriverReadDrive;
    Fat32Startup.Write = DriverWriteDrive;



    if(!Fat32DriverInit(&Fat32Startup)) ERROR_EXIT("FAT32_DRIVER_INIT_FAILED.");

    printf("LOADING_FILES\n");
    HANDLE BootSector = CreateFileA(BOOT_SECT, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE BootStage2 = CreateFileA(BOOT_STAGE2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE OriginalPartition = CreateFileA(ORIGINAL_PARTITION, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(BootSector == INVALID_HANDLE_VALUE || BootStage2 == INVALID_HANDLE_VALUE || OriginalPartition == INVALID_HANDLE_VALUE) {
        printf("Failed to Open BOOT_SECTOR OR BOOT_STAGE2 OR ORIGINAL_PARTITION");
        return -1;
    }
    UINT64 SizeBootSect = GetFileSize(BootSector, NULL), SizeBootStage2 = GetFileSize(BootStage2, NULL), SizeOriginalPartition = GetFileSize(OriginalPartition, NULL);

    UINT64 DriveSize = 0;
    DWORD DwHigh = 0;
    DriveSize = GetFileSize(OsImage, &DwHigh);
    DriveSize |= (UINT64)DwHigh << 32;

    printf("Drive Size : %I64x, BOOT_SECTOR_SIZE : %llu \n", DriveSize, SizeBootSect);

    if(SizeBootSect != BOOT_CODE_LENGTH) {
        printf("Invalid boot sector");
        return -1;
    }
    if(SizeBootStage2 != BOOTMGR_SIZE) {
        printf("Invalid BOOT_MGR");
        return -1;
    }

    void* BufferBootStage2 = malloc(SizeBootStage2);
    if(!BufferBootStage2) return -2;

    HANDLE PartBootsect = CreateFileA(PARTITION_BOOT_SECTOR, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if(PartBootsect == INVALID_HANDLE_VALUE) ERROR_EXIT("Failed to open Partition BootSector");

    UINT PartbootsectSize = GetFileSize(PartBootsect, NULL);
    if(PartbootsectSize != 0x200) ERROR_EXIT("INVALID_PARTITION_BOOT_SECTOR");

    char* PartitionBootsector = malloc(0x200);
    if(!ReadFile(PartBootsect, PartitionBootsector, 0x200, NULL, NULL)) ERROR_EXIT("Failed to read partition bootsector");

    printf("READING_FILES\n");

    char FsMbr[0x200] = {0};

    ReadFile(BootSector, Image.Mbr.BootCode, BOOT_CODE_LENGTH, NULL, NULL);
    ReadFile(BootStage2, &Image.BootManagerCode, BOOTMGR_SIZE, NULL, NULL);
    
    printf("SETTING_UP_FS\n");
    Image.Mbr.BootSignature = 0xAA55;
    struct MBR_PARTITION_ENTRY* GptPartition = &Image.Mbr.Partitions[0];

    
    // struct FAT32_BOOTSECTOR* BufferOriginalPartition = malloc(SizeOriginalPartition + 0x200);
    // if((UINT64)BufferOriginalPartition & 0x1FF) (char*)BufferOriginalPartition += (0x200 - (UINT64)BufferOriginalPartition & 0x1FF);
    // if(!BufferOriginalPartition) return -1;
    // ReadFile(OriginalPartition, BufferOriginalPartition, SizeOriginalPartition, NULL, NULL);
    
    UINT32 NumSectors = 0x10000; // 80 MB

    Image.Partitions[0].FirstLba = IMAGE_START_LBA;
    // Image.Gpt.CRC32_1 = RtlCrc32(Image.Partitions, sizeof(GUID_PARTITION_ENTRY), 0);
    // Image.Gpt.CRC32 = RtlCrc32(&Image.Gpt, sizeof(GUID_PARTITION_TABLE), 0);


    // Setting UP Hybrid boot partitions (MBR Only compatible partitions)


    



    printf("WRITING_FILES\n");
    UINT ImageSectors = sizeof(Image) >> 9;
    if(sizeof(Image) & 0x1FF) ImageSectors++;
    SetFilePointer(OsImage, 0, NULL, FILE_BEGIN);

    OVERLAPPED Overlapped = {0};



    

    Overlapped.Offset = IMAGE_START_LBA << 9;
    if(SizeOriginalPartition & 0x1FF) {
        SizeOriginalPartition += 0x200;
        SizeOriginalPartition &= ~(0x1FF);
    } 

    

    Overlapped.Offset = (IMAGE_START_LBA + EXTENDED_BOOT_AREA_SIZE) << 9;

    // if(!WriteFile(OsImage, (char*)BufferOriginalPartition + (BufferOriginalPartition->fatpb.dos_bpb.dos2bpb.reserved_area_size << 9), SizeOriginalPartition - (BufferOriginalPartition->fatpb.dos_bpb.dos2bpb.reserved_area_size << 9), NULL, &Overlapped)) ERROR_EXIT("Failed to write data to the drive.");


    // Boot Sector with Extended Area size modified
    // UINT OldBsCopy = BufferOriginalPartition->fatpb.fat32_bscopy;
    

    Partition = Fat32CreatePartition(
        IMAGE_START_LBA, NumSectors, 1, 
        "FATPART", EXTENDED_BOOT_AREA_SIZE, PartitionBootsector);


    Image.Partitions[0].LastLba =  IMAGE_START_LBA + Partition->BootSector.DosParamBlock.TotalSectors;

    SetFilePointer(OsImage, (IMAGE_START_LBA + Partition->BootSector.DosParamBlock.TotalSectors) << 9, NULL, FILE_BEGIN);
    SetEndOfFile(OsImage);


    if(!Partition) ERROR_EXIT("FAILED_TO_CREATE_PARTITION");
    Fat32WriteReservedArea(Partition, 0, BOOTMGR_SIZE >> 9, Image.BootManagerCode);

    // Creating System Files

    CopyDirectoryFiles(L"../kernel/iso", L"");

    printf("FS_WRITTEN\n");


    struct MBR_PARTITION_ENTRY* HybridPartition = &Image.Mbr.Partitions[1];
    HybridPartition->Status = 0x80;
    HybridPartition->StartingChsAddress.Sector = IMAGE_START_LBA + 1;
    HybridPartition->PartitionType = FAT32_LBA_CHS; // 0xC Only uses LBA 0xB to use also CHS
    HybridPartition->EndingChsAddress.Sector = 0x7F;
    HybridPartition->EndingChsAddress.Head = 0xFE;
    HybridPartition->EndingChsAddress.Cylinder = 0x80;

    HybridPartition->LbaStart = IMAGE_START_LBA;
    HybridPartition->NumSectors = Partition->BootSector.DosParamBlock.TotalSectors;
    
    SetFilePointer(OsImage, 0, NULL, FILE_BEGIN);
    WriteFile(OsImage, &Image.Mbr, 0x200, NULL, NULL);

    printf("BOOT_HDR_WRITTEN.\n");

    UINT64 BPTSize = sizeof(BOOT_POINTER_TABLE) + sizeof(BOOT_POINTER_TABLE_ENTRY) * NUM_BOOT_POINTER_ENTRIES;
    if(BPTSize & 0x1FF) {
        BPTSize += 0x1FF;
        BPTSize &= ~(0x1FF);
    }

    BOOT_POINTER_TABLE* BootPointerTable = malloc(BPTSize);
    memset(BootPointerTable, 0, BPTSize);
    BootPointerTable->Magic = 0xEEF7C8D9;
    BootPointerTable->StrMagic[0] = 'P';
    BootPointerTable->StrMagic[1] = 'R';
    BootPointerTable->StrMagic[2] = 'T';
    BootPointerTable->StrMagic[3] = 'P';

    BootPointerTable->BootLoaderMajorVersion = BOOT_MAJOR;
    BootPointerTable->BootLoaderMinorVersion = BOOT_MINOR;
    BootPointerTable->EntrySize = sizeof(BOOT_POINTER_TABLE_ENTRY);
    BootPointerTable->NumEntries = NUM_BOOT_POINTER_ENTRIES;
    BootPointerTable->OffsetToEntries = (char*)BootPointerTable->Entries - (char*)BootPointerTable;
    // MAJOR Kernel Entry

    FAT_FILE_INFORMATION FileInformation = {0};

    if(!Fat32GetFileInformation(Partition, KERNEL_PATH, &FileInformation)) ERROR_EXIT("MAJOR_KERNEL_ENTRY Setup Failed.")

    UINT32 NumClusters = FileInformation.FileSize / (Partition->BootSector.BiosParamBlock.ClusterSize << 9);
    if(FileInformation.FileSize % (Partition->BootSector.BiosParamBlock.ClusterSize << 9)) NumClusters++;

    BootPointerTable->KernelClusterStart = FileInformation.ClusterStart;
    BootPointerTable->KernelFileNumClusters = NumClusters;
    BootPointerTable->AllocationTableLba = Partition->BootSector.DosParamBlock.HiddenSectors + Partition->BootSector.BiosParamBlock.ReservedAreaSize;
    // DataClustersLba - 2 Clusters(Cluster begins at 2) for EMULATED_CLUSTER_CHAIN in FAT Format
    BootPointerTable->DataClustersLba = BootPointerTable->AllocationTableLba + Partition->BootSector.BiosParamBlock.FatCount * Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat - 2 * Partition->BootSector.BiosParamBlock.ClusterSize;
    for(UINT64 i = 0;i<NUM_BOOT_POINTER_ENTRIES;i++) {
        if(!Fat32GetFileInformation(Partition, BootPointerPaths[i], &FileInformation)) ERROR_EXIT("BOOT_POINTER_ENTRY Setup Failed.");
        BootPointerTable->Entries[i].ClusterStart = FileInformation.ClusterStart;
        BootPointerTable->Entries[i].NumClusters = FileInformation.FileSize / (Partition->BootSector.BiosParamBlock.ClusterSize << 9);
        if(FileInformation.FileSize % (Partition->BootSector.BiosParamBlock.ClusterSize << 9)) BootPointerTable->Entries[i].NumClusters++;
        BootPointerTable->Entries[i].PathLength = StrlenW(BootPointerPaths[i]);
        memcpy(BootPointerTable->Entries[i].Path, BootPointerPaths[i], BootPointerTable->Entries[i].PathLength << 1);
    }

    SetFilePointer(OsImage, (IMAGE_START_LBA + 1 + (BOOTMGR_SIZE >> 9)) << 9, NULL, FILE_BEGIN);
    if(!WriteFile(OsImage, BootPointerTable, BPTSize, NULL, NULL)) ERROR_EXIT("Failed to write BOOT_POINTER_TABLE.");


    printf("Image Setup Success.\n");
    system("pause");
    return 0;
}