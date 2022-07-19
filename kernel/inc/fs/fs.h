#pragma once

#include <fs/mbr.h>
#include <stdint.h>
#include <CPU/process.h>
#include <stddef.h>
#include <dsk/disk.h>
#include <fs/dir.h>
#define FAT_EXTENDED_BOOT_SIG 0x29
#define FILE_ENTRIES_MAX 120
#define FS_LS_MAX 0xFFFFFFFFFFFFFFFF
// struct FAT12_16_BOOTSECTOR{
//     struct FAT_HEADER header;
//     uint8_t bios_int13;
//     uint8_t unused0;
//     uint8_t extended_boot_sig; // to validate next three fields (0x29)
//     uint32_t volume_serial_number;
//     char volume_label[11];
//     char file_system_type_level[8];
//     unsigned char unused1[448];
//     uint16_t boot_signature; // 0xAA55
// };
enum FAT_FILE_ATTRIBUTES{
    FAT_ATTR_READ_ONLY = 1,
    FAT_ATTR_HIDDEN = 2,
    FAT_ATTR_SYSTEM_FILE = 4,
    FAT_ATTR_VOLUME_LABEL = 8,
    FAT_ATTR_LONG_FILE_NAME = 0x0F,
    FAT_ATTR_DIR = 0x10,
    FAT_ATTR_ARCHIVE = 0x20
};






#pragma pack(push, 1)

struct FAT_DIRECTORY_ENTRY{
    uint8_t allocation_status; // 0 unallocated, 0xe5 deleted
    char file_name[10]; // char 2 to 11 "." implied between bytes 7 and 8
    uint8_t file_attributes;
    uint8_t reserved;
    uint8_t file_creation_time; // in tenths of seconds
    uint16_t creation_time; // hours, minutes, seconds
    uint16_t creation_date; // day, month, year
    uint16_t access_date;
    uint16_t first_cluster_addr_high; // 0 for fat12/fat16
    uint16_t modified_time; // hours, minutes, seconds
    uint16_t modified_date;
    uint16_t first_cluster_addr_low;
    uint32_t file_size; // 0 for dirs
};
struct FAT_LFN_DIR_ENTRY{
    uint8_t sequence_number; //  (ORed with 0x40) and allocation status (0xe5 if unallocated)
    wchar_t file_name_low[5]; // File name characters 1-5 (Unicode)
    uint8_t file_attributes; // 0x0f
    uint8_t reserved0;
    uint8_t checksum;
    wchar_t file_name_mid[6]; // unicode 6-11
    uint16_t reserved1;
    wchar_t file_name_high[2]; // unicode 12-13
};


#pragma pack(pop)


// System Disk Utility Functions




KERNELSTATUS KERNELAPI GetFileInfo(FILE File, FILE_INFORMATION_STRUCT* FileInformation, LPWSTR FileName);
KERNELSTATUS KERNELAPI ReadFile(FILE File, UINT64 Offset, UINT64* NumBytesRead, void* Buffer);


int FsMountDevice(RFDEVICE_OBJECT device);


// unsigned char fdirls(const wchar_t* _path, struct DIR_LS** _out);

KERNELSTATUS KERNELAPI ListFileContent(FILE File, DIRECTORY_FILE_LIST* FileList, UINT64 MaxFileCount);



FILE KERNELAPI OpenFile(LPWSTR Path, UINT64 Access, FILE_INFORMATION_STRUCT* FileInformation);
KERNELSTATUS KERNELAPI CloseFile(FILE File);

KERNELSTATUS KERNELAPI FsReadDrive(DISK_DEVICE_INSTANCE* Drive, UINT64 SectorOffset, UINT64 SectorCount, void* Buffer);
KERNELSTATUS KERNELAPI FsReadPartitionSectors(PARTITION_INSTANCE* Partition, UINT64 SectorOffset, UINT64 SectorCount, void* Buffer);
KERNELSTATUS KERNELAPI FsReadPartition(PARTITION_INSTANCE* Partition, UINT64 ClusterOffset, UINT64 ClusterCount, void* Buffer);

KERNELSTATUS KERNELAPI DefaultReadPartitionSectors(PARTITION_INSTANCE* Partition, UINT64 SectorOffset, UINT64 SectorCount, void* Buffer);
KERNELSTATUS KERNELAPI DefaultWritePartitionSectors(PARTITION_INSTANCE* Partition, UINT64 SectorOffset, UINT64 SectorCount, void* Buffer);

KERNELSTATUS KERNELAPI DefaultCloseFile(FILE File);
FILE KERNELAPI DefaultOpenFile(PARTITION_INSTANCE* Partition, LPWSTR Path, UINT64 Access, FILE_INFORMATION_STRUCT* FileInformation);
FILE KERNELAPI FindOpenFilePath(OPEN_FILE_LIST* List, LPWSTR Path);
//void escape_slash(wchar_t* path, wchar_t** out);

//struct VIRTUAL_PARTITION_INSTANCE* FsResolvePath(wchar_t* path, wchar_t** out);