#include <fs/fat32/fat32.h>
#include <cstr.h>
#include <preos_renderer.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <fs/fs.h>
#include <dsk/ata.h>
#include <kernel.h>
#include <stdlib.h>
#include <fs/fat32/interface.h>

static inline int FAT32_CheckBootsector(struct FAT32_BOOTSECTOR* bootsect, uint64_t lba_base){
    if(bootsect->fatpb.dos_bpb.dos2bpb.sector_size != 512 ||
    bootsect->boot_signature != MBR_BOOT_SIGNATURE ||
    bootsect->fatpb.extended_boot_signature != 0x29 ||
    bootsect->fatpb.root_dir_start < 2 ||
    !bootsect->fatpb.fat32_bscopy ||
    !bootsect->fatpb.fs_info_lba ||
    bootsect->fatpb.sectors_per_fat < FAT32_MIN_SECTORS_PER_FAT ||
    bootsect->fatpb.dos_bpb.hidden_sectors != lba_base ||
    bootsect->fatpb.dos_bpb.total_sectors < FAT32_MIN_SECTORS ||
    bootsect->fatpb.dos_bpb.dos2bpb.cluster_size < 1 ||
    bootsect->fatpb.dos_bpb.dos2bpb.fat_count < 1
    ) return -1;

    return SUCCESS;
}

static inline BOOL FAT32_SetVolumeLabelFromDirList(DIRECTORY_FILE_LIST* FileList, PARTITION_INSTANCE* Partition){
    DIR_LIST_FILE_ENTRY* Entry = NULL;
    while (FsDirNext(FileList, &Entry) == 0) {
        if(Entry->attributes & FAT_ATTR_VOLUME_LABEL){
            if (!SetPartitionName(Partition, Entry->FileName)) SET_SOD_MEDIA_MANAGEMENT;
            return TRUE;
        }
    }
    return FALSE;
}
void FAT32_Mount(UINT64 SectorBase, UINT64 SectorCount, DISK_DEVICE_INSTANCE* Disk){
    
    struct FAT32_BOOTSECTOR* bootsect = kpalloc(1);
    if(!bootsect) SET_SOD_OUT_OF_RESOURCES;
    FsReadDrive(Disk, SectorBase, 1, bootsect);
    
    if(FAILED(FAT32_CheckBootsector(bootsect, SectorBase))) {
        free(bootsect,kproc);
        return;
    }

    

    DISK_INFO PartitionInfo = { 0 };
    PartitionInfo.BaseAddress = SectorBase;
    PartitionInfo.SectorsInCluster = bootsect->fatpb.dos_bpb.dos2bpb.cluster_size;
    PartitionInfo.BytesInCluster = PartitionInfo.SectorsInCluster << 9; // * 512
    PartitionInfo.FsType = MBR_FAT32;
    PartitionInfo.MaxAddress = SectorCount; // Physical Max Addr = MaxAddr + BaseAddr

    struct FAT32_FS_DESCRIPTOR* fs_desc = kmalloc(sizeof(struct FAT32_FS_DESCRIPTOR));
    if (!fs_desc) SET_SOD_MEMORY_MANAGEMENT;
    PARTITION_MANAGEMENT_INTERFACE Interface = { 0 };
    Interface.SectorRawRead = DefaultReadPartitionSectors;
    Interface.SectorRawWrite = DefaultWritePartitionSectors;
    Interface.RawRead = FAT32i_RawPartitionRead;
    Interface.RawWrite = FAT32i_RawPartitionWrite;
    Interface.GetInfo = FAT32_FsGetFileInfo;
    Interface.Read = FAT32_FsRead;
    Interface.ListFileContent = FAT32_FsListDirectory;
    Interface.Validate = ValidateFile;
    Interface.Open = DefaultOpenFile;
    Interface.Close = DefaultCloseFile;

    fs_desc->fat_count = bootsect->fatpb.dos_bpb.dos2bpb.fat_count;
    fs_desc->fat_size = bootsect->fatpb.sectors_per_fat;
    fs_desc->reserved_area_size = bootsect->fatpb.dos_bpb.dos2bpb.reserved_area_size;
    fs_desc->rootdir_cluster = bootsect->fatpb.root_dir_start;

    PARTITION_INSTANCE* Partition = CreatePartition(NULL, NULL, PARTITION_DEFAULT_SECURITY_DESCRIPTOR,
        Disk, &PartitionInfo, fs_desc, &Interface
    );

    if (!Partition) SET_SOD_MEDIA_MANAGEMENT;

    
    
    free(bootsect,kproc);
    
    DIRECTORY_FILE_LIST FileList = { 0 };
    
    if (!FsCreateDirList(kproc, &FileList)) SET_SOD_MEDIA_MANAGEMENT;

    
    FILE Root = Partition->Interface.Open(Partition, L"/", FILE_OPEN_LIST_CONTENT, NULL);

    

    // Partition Name Must be within the first 10 file entries
    if(!Root ||
        KERNEL_ERROR(ListFileContent(Root, &FileList, 10))
    ){
        
        DestroyPartition(Partition);
        FsReleaseDirList(kproc, &FileList);
        SET_SOD_MEDIA_MANAGEMENT;
    }
    FAT32_SetVolumeLabelFromDirList(&FileList, Partition);
    FsReleaseDirList(kproc, &FileList);

    
}