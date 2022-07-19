#include <fs/fat32/fileinfo.h>
#include <stdlib.h>
#include <MemoryManagement.h>
#include <kernel.h>
#include <fs/fat32/fat32.h>
#include <fs/fs.h>
#include <fs/fat32/dir.h>
#include <preos_renderer.h>
#include <cstr.h>
#include <interrupt_manager/SOD.h>
#include <fs/fat32/stream.h>
#define GetCluster(x) (((struct FAT_DIRECTORY_ENTRY*)x)->first_cluster_addr_low | (((struct FAT_DIRECTORY_ENTRY*)x)->first_cluster_addr_high << 16))

KERNELSTATUS KERNELAPI FAT32_FsGetFileInfo(FILE File, FILE_INFORMATION_STRUCT* FileInformation, LPWSTR FileName){
    if(!ValidateFile(File) || !FileInformation) return KERNEL_SERR_INVALID_PARAMETER; // invalid parameter

    struct FAT32_FS_DESCRIPTOR* fsdesc = File->Partition->FsInfo;
    wchar_t PathFileName[MAX_FILE_NAME] = { 0 };
    wchar_t EntryName[MAX_FILE_NAME] = { 0 };

    struct FAT_DIRECTORY_ENTRY* Entry = NULL;
    UINT64 FileNameLength = 0;
    UINT64 EntryNameLength = 0;
    LPWSTR Path = File->Path;
    BOOL Found = FALSE;
    FAT_READ_STREAM* Stream = FatOpenReadStream(fsdesc->rootdir_cluster, 0, NULL, File->Partition);
    if (!Stream) SET_SOD_MEDIA_MANAGEMENT;
    while ((Path = ResolveNextFileName(Path, PathFileName, &FileNameLength))) {
        while (FatReadNextFileEntry(&Entry, Stream, EntryName, &EntryNameLength)) {
            if (EntryNameLength == FileNameLength && wstrcmp_nocs(EntryName, PathFileName, FileNameLength)) {
                struct FAT_DIRECTORY_ENTRY* LastEntry = FatLastFileEntry(Entry);
                if (LastEntry->file_attributes & FAT_ATTR_VOLUME_LABEL) continue;
                if (*Path) {
                    FatCloseReadStream(Stream);
                    Stream = FatOpenReadStream(GetCluster(LastEntry), 0, NULL, File->Partition);
                    if (!Stream) SET_SOD_MEDIA_MANAGEMENT;
                    Found = TRUE;
                    Entry = NULL;

                    break;
                }
                else {
                    FileInformation->Cluster = GetCluster(LastEntry);
                    FileInformation->PartitionId = File->Partition->PartitionId;
                    FileInformation->DiskId = File->Partition->Disk->DiskId;
                    FileInformation->FileSize = LastEntry->file_size;
                    FileInformation->FileSystem = MBR_FAT32;
                        
                    UINT64 EntryAttributes = LastEntry->file_attributes;
                    UINT64 Attributes = 0;
                    if (EntryAttributes & FAT_ATTR_DIR) Attributes |= FILE_ATTRIBUTE_DIRECTORY;
                    if (EntryAttributes & FAT_ATTR_HIDDEN) Attributes |= FILE_ATTRIBUTE_HIDDEN;
                    if (EntryAttributes & FAT_ATTR_READ_ONLY) Attributes |= FILE_ATTRIBUTE_READ_ONLY;
                    if (EntryAttributes & FAT_ATTR_SYSTEM_FILE) Attributes |= FILE_ATTRIBUTE_SYSTEM_FILE;
                    FileInformation->FileAttributes = Attributes;
                    return KERNEL_SOK;
                }
            }
        }
        if (!Found) {
            FatCloseReadStream(Stream);
            return KERNEL_SERR_NOT_FOUND;
        }
        Found = FALSE;
    }

    // if path is root

    FileInformation->Cluster = fsdesc->rootdir_cluster;
    FileInformation->PartitionId = File->Partition->PartitionId;
    FileInformation->DiskId = File->Partition->Disk->DiskId;
    FileInformation->FileSize = 10;
    FileInformation->FileSystem = MBR_FAT32;
    FileInformation->FileAttributes = 0;

    return KERNEL_SOK;
}