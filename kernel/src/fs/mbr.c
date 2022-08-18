#include <fs/fs.h>
#include <preos_renderer.h>
#include <cstr.h>

#include <fs/fat32/fat32.h>
// #include <fs/ntfs.h>


#include <MemoryManagement.h>
unsigned char PartitionTypes[] = {
    0x00,0x01,4,5,6,7,0x0b,0x0c,0x0e,
    0x0f,0x11,0x14,0x16,0x1b,0x1c,
    0x1e,0x42,0x82,0x83,0x84,0x85,
    0x86,0x87,0xa0,0xa1,0xa5,0xa6,
    0xa8,0xa9,0xab,0xb7,0xb8,0xee,
    0xef,0xfb,0xfc
};

char* PartitionTypeStrings[] = {
    "Empty","FAT12, CHS", "FAT16, 16-32 MB, CHS",
    "Microsoft Extended, CHS", "FAT16, 32MB-2GB, CHS",
    "NTFS", "Unknown", "Unknown", "Unknown", "FAT32, CHS", "FAT32, LBA", "FAT16, 32MB-2GB, LBA",
    "Hidden FAT12, CHS", "Hidden FAT16, 16-32 MB, CHS","Hidden FAT16, 32 MB-2 GB, CHS",
    "Hidden FAT32, CHS", "Hidden FAT32, LBA", "Hidden FAT16, 32 MB-2 GB, LBA",
    "Microsoft MBR. Dynamic Disk", "Solaris x86", "Linux Swap", "Linux",
    "Hibernation", "Linux Extended", "NTFS Volume Set", "NTFS Volume Set", "Hibernation",
    "Hibernation", "FreeBSD", "OpenBSD", "Mac OSX", "NetBSD", "Mac OSX Boot", "	BSDI", "BSDI swap",
    "EFI GPT Disk", "EFI System Partition", "Vmware File System", "Vmware swap"
};

int MountMbrDevice(DISK_DEVICE_INSTANCE* Disk, void* sect0){
    struct MBR_HDR* hdr = (struct MBR_HDR*)sect0;

    if(hdr->boot_signature != MBR_BOOT_SIGNATURE || !sect0) return -1;
    for(uint8_t i = 0;i<4;i++){
        switch(hdr->partition_entries[i].partition_type){
            case MBR_NTFS:
            {
                break;
            }
            case MBR_FAT32:
            {
            
                // FAT32_Mount(
                // hdr->partition_entries[i].starting_lba_addr_low | (hdr->partition_entries[i].starting_lba_addr_high << 16),
                // hdr->partition_entries[i].sector_count_low | (hdr->partition_entries[i].sector_count_high<<16),
                // Disk);
                break;
            }
            case MBR_GPT:
            {
                break;
            }
        }
    }
    return SUCCESS;
}
