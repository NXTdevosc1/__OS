#define __DLL_EXPORTS
#include <fat32.h>

static UINT16 EntryName[0x400] = {0};
static UINT16 ResultPath[0x400] = {0};


__FS_BOOL FSBASEAPI Fat32GetFileInformation(PFAT32_PARTITION Partition, UINT16* FilePath, FAT_FILE_INFORMATION* FileInformation) {
    int Status = 0;
    UINT32 Cluster = Partition->BootSector.ExtendedBiosParamBlock.RootDirStart;
    FAT32_FILE_ENTRY Entry = {0};
    UINT16* PIndexedPath = ResultPath;



    while((Status = Fat32ParsePath(FilePath, &FilePath, EntryName)) != 0) {
        if(!Fat32FindEntry(Partition, EntryName, Cluster, &Entry)) return 0;
        if(Status == 2) {
            FileInformation->ClusterStart = (UINT32)Entry.FirstClusterLow | ((UINT32)Entry.FirstClusterHigh << 16);
            UINT16* en = EntryName;
            UINT16* fn = FileInformation->FileName;
            while(*en) {
                *fn = *en;
                en++;
                fn++;
            }
            *fn = 0;
            FileInformation->FatAttributes = Entry.FileAttributes;
            /*TODO : CREATION/MODIFICATION TIME & DATE*/
            FileInformation->ParentDirectoryCluster = Cluster;
            FileInformation->FileSize = Entry.FileSize;
            return 1;
        }else {
            Cluster = (UINT32)Entry.FirstClusterLow | ((UINT32)Entry.FirstClusterHigh << 16);
        }
    }
    return 0;
}