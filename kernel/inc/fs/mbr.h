#pragma once
#include <stdint.h>
#include <IO/pcie.h>
#include <dsk/disk.h>
#include <stddef.h>
#include <fs/gpt.h>
#define MBR_BOOT_SIGNATURE 0xAA55

enum MBR_PARTITION_TYPE{
    MBR_FAT32 = 0x0C,
    MBR_NTFS = 0x07,
    MBR_GPT = 0xEE
};


struct MBR_PARTITION_ENTRY{
    uint8_t bootable;
    uint16_t starting_chs_addr;
    uint8_t partition_type;
    uint16_t ending_chs_addr;
    uint16_t starting_lba_addr_low;
    uint8_t starting_lba_addr_high;
    uint16_t sector_count_low;
    uint8_t sector_count_high;
};

struct MBR_HDR{
    uint8_t boot_code[445];
    struct MBR_PARTITION_ENTRY partition_entries[4];
    uint16_t boot_signature;
};

int MountMbrDevice(DISK_DEVICE_INSTANCE* Disk, void* sect0);