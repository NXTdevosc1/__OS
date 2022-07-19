#define __DLL_EXPORTS
#include <fat32.h>



static UINT16 EntryName[0x400] = {0};
static UINT16 ResultPath[0x400] = {0};
int FSBASEAPI Fat32CreateFile(PFAT32_PARTITION Partition, UINT16* Path, FAT_FILE_ATTRIBUTES FileAttributes){
    int Status = 0;
    UINT32 Cluster = Partition->BootSector.ExtendedBiosParamBlock.RootDirStart;
    FAT32_FILE_ENTRY Entry = {0};
    UINT16* PIndexedPath = ResultPath;


    while((Status = Fat32ParsePath(Path, &Path, EntryName)) != 0) {
        
        UINT16* PIndexedEntryName = EntryName;
        

        if(Status == 2) {
            Status = Fat32AllocateEntry(Partition, Cluster, EntryName, FileAttributes);
            if(Status != FAT_SUCCESS) {
                return Status;
            }
            StartupInfo.Print("File %ls Created Successfully\n", EntryName);
            return FAT_SUCCESS;
        } else {
            *PIndexedPath = L'/';
            *PIndexedPath++;
            while(*PIndexedEntryName) {
                *PIndexedPath = *PIndexedEntryName;
                PIndexedPath++;
                PIndexedEntryName++;
            }
            *PIndexedPath = 0;
            if(!Fat32FindEntry(Partition, EntryName, Cluster, &Entry)) {
                return FAT_NOT_FOUND;
            }
            if(!(Entry.FileAttributes & FAT_DIRECTORY)) return FAT_NOT_FOUND;
            Cluster = (UINT32)Entry.FirstClusterLow | ((UINT32)Entry.FirstClusterHigh << 16);
            if(!Cluster) {
                Cluster = Fat32AllocateClusterChain(Partition, 0, 1);
                if(!Cluster) return FAT_NO_ENOUGH_SPACE;
                Fat32MemClear(Partition->ReadCluster, Partition->BootSector.BiosParamBlock.ClusterSize << 9);
                Fat32WriteCluster(Partition, Cluster, Partition->ReadCluster);
                if(!Fat32EditFile(Partition, ResultPath, FILE_EDIT_CLUSTER, 0, Cluster, 0)) return FAT_WRITE_ERROR;
            }
        }
    }
    
    return FAT_NOT_FOUND;
}