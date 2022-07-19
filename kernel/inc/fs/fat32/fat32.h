#pragma once
#include <stdint.h>
#include <fs/mbr.h>
#include <dsk/disk.h>
#include <fs/fat32/ls.h>
#include <fs/fat32/fileinfo.h>
#include <fs/fat32/read.h>
#include <fs/fat32/stream.h>
#define FAT32_EXTENDED_SIGNATURE 0x29
#define FAT32_MIN_SECTORS_PER_FAT 20
#define FAT32_MIN_SECTORS 100
#define FAT32_END_OF_CHAIN 0xFFFFFF0
#define FAT32_CLUSTER_MASK 0xFFFFFFF

#pragma pack(push, 1)

struct BIOS_PARAMETER_BLOCK{
    uint16_t sector_size;
    uint8_t cluster_size;
    uint16_t reserved_area_size;
    uint8_t fat_count;
    uint16_t max_root_dir_entries; // not set in fat32, just for fat12, fat16
    uint16_t total_sectors; // not set in fat32
    uint8_t media_descriptor;
    uint16_t sectors_per_fat; // not set in fat32
};

struct DOS3_PARAMETER_BLOCK{
    struct BIOS_PARAMETER_BLOCK dos2bpb;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors; //Count of hidden sectors preceding the partition that contains this FAT volume.
    uint32_t total_sectors;
};
struct FAT32_EXTENDED_BIOS_PARAMETER_BLOCK{
    struct DOS3_PARAMETER_BLOCK dos_bpb;
    uint32_t sectors_per_fat;
    uint16_t drive_description;
    uint16_t version;
    uint32_t root_dir_start;
    uint16_t fs_info_lba;
    uint16_t fat32_bscopy; //First logical sector number of a copy of the three FAT32 boot sectors, typically 6.
    uint8_t reserved[12];
    uint8_t physical_drive_number;
    uint8_t unused; // for FAT12/FAT16 (Used for various purposes; see FAT12/FAT16)
    uint8_t extended_boot_signature; // 0x29
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fs_label[8];
};


struct FAT32_FSINFO{
    uint32_t fsinfo_sig0; // RRaA
    uint8_t reserved0[480];
    uint32_t fsinfo_sig1; // rrAa
    uint32_t free_clusters; // last known number of free data clusters on the volume
    uint32_t recently_allocated_cluster;
    uint8_t reserved1[12];
    uint16_t signull;
    uint16_t signature; // 0x55AA
};

struct FAT32_BOOTSECTOR {
    uint8_t jmp[3];
    uint8_t oem_name[8];
    struct FAT32_EXTENDED_BIOS_PARAMETER_BLOCK fatpb;
    uint8_t boot_code[420];
    uint16_t boot_signature;
};

struct FAT32_FS_DESCRIPTOR{
    uint32_t allocated_clusters;
    uint16_t fat_size;
    uint16_t fat_count;
    uint16_t reserved_area_size;
    uint32_t rootdir_cluster;
};
#pragma pack(pop)
void FAT32_Mount(UINT64 SectorBase, UINT64 SectorCount, DISK_DEVICE_INSTANCE* Disk);
