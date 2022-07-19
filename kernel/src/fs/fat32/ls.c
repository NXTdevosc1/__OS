#include <fs/fat32/ls.h>
#include <fs/fat32/fat32.h>
#include <fs/fat32/dir.h>
#include <stdlib.h>
#include <MemoryManagement.h>
#include <kernel.h>
#include <preos_renderer.h>
#include <interrupt_manager/SOD.h>
#include <fs/fs.h>
#include <fs/fat32/stream.h>

#include <cstr.h>


#define GetCluster(x) (((struct FAT_DIRECTORY_ENTRY*)x)->first_cluster_addr_low | (((struct FAT_DIRECTORY_ENTRY*)x)->first_cluster_addr_high << 16))
KERNELSTATUS KERNELAPI FAT32_FsListDirectory(FILE File, DIRECTORY_FILE_LIST* FileList, UINT64 MaxFileCount){    if (!ValidateFile(File) || !FileList) return KERNEL_SERR_INVALID_PARAMETER;
if (!ValidateFile(File) || !FileList) return KERNEL_SERR_INVALID_PARAMETER;
    FileList->file_count = 0;
    
    struct FAT32_FS_DESCRIPTOR* fsd = File->Partition->FsInfo;
    
    struct FAT_DIRECTORY_ENTRY* Entry = NULL;
    
    wchar_t FileName[MAX_FILE_NAME] = { 0 };
    wchar_t EntryName[MAX_FILE_NAME] = { 0 };

    

    LPWSTR Path = File->Path;
    UINT64 FileNameLength = 0;
    UINT64 EntryNameLength = 0;

    BOOL Found = FALSE;
    
    FAT_READ_STREAM* Stream = FatOpenReadStream(fsd->rootdir_cluster, 0, NULL, File->Partition);
    if (!Stream) SET_SOD_MEDIA_MANAGEMENT;

    UINT64 iy = 0;

    while ((Path = ResolveNextFileName(Path, FileName, &FileNameLength))) {
        
        Gp_draw_sf_textW(Path, 0xffffff, 20, 20 + iy * 20);
        Gp_draw_sf_textW(FileName, 0xffffff, 350, 20 + iy * 20);
        Gp_draw_sf_textW(L"Read", 0xffffff, 500, 20 + iy * 20);
        iy++;
        continue;
        while (FatReadNextFileEntry(&Entry, Stream, EntryName, &EntryNameLength)) {
            if (EntryNameLength == FileNameLength && wstrcmp_nocs(EntryName, FileName, FileNameLength)) {
                struct FAT_DIRECTORY_ENTRY* LastEntry = FatLastFileEntry(Entry);
                if (LastEntry->file_attributes & FAT_ATTR_VOLUME_LABEL) continue;

                FatCloseReadStream(Stream);
                Stream = FatOpenReadStream(GetCluster(LastEntry), 0, NULL, File->Partition);
                if (!Stream) SET_SOD_MEDIA_MANAGEMENT;
                Found = TRUE;
                break;
            }
        }
        if (!Found) {
            FatCloseReadStream(Stream);
            return KERNEL_SERR_NOT_FOUND;
        }
        Found = FALSE;
    }
    // Read Resolved Directory
    FILE_CONTENT_LIST* List = &FileList->ls;
    UINT64 Index = 0;
    while (FileList->file_count < MaxFileCount && FatReadNextFileEntry(&Entry, Stream, FileName, &FileNameLength)) {
        struct FAT_DIRECTORY_ENTRY* Data = FatLastFileEntry(Entry);
        List->Files[Index].Set = TRUE;
        List->Files[Index].attributes = Data->file_attributes;
        List->Files[Index].FileSize = Data->file_size;
        List->Files[Index].StartCluster = GetCluster(Data);

        LPWSTR name = kmalloc((FileNameLength + 1) << 1);
        if (!name) SET_SOD_MEMORY_MANAGEMENT;
        memcpy16(name, FileName, FileNameLength);
        name[FileNameLength] = 0;

      
        List->Files[Index].FileName = name;
        FileList->file_count++;
        Index++;
        if (Index == MAX_FILES_PER_DIRLS_LIST) {
            if (!List->Next) {
                List->Next = kmalloc(sizeof(*List));
                SZeroMemory(List->Next);
            }
            List = List->Next;
            Index = 0;
        }
    }
    return KERNEL_SOK;
}