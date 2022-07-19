#define __DLL_EXPORTS
#include <fat32.h>
static UINT16 EntryName[0x400] = {0};
static UINT16 FindDirBuffer[0x400] = {0};

__FS_BOOL FSBASEAPI Fat32EditFile(PFAT32_PARTITION Partition, UINT16* Path, FILE_EDIT_PARAMETERS FileEditParameters, FAT_FILE_ATTRIBUTES NewAttributes, UINT32 ClusterFileSize, UINT16 NewModificationDate) {
    // Find File Path
    if(!FileEditParameters) return 0;
    int Status = 0;
    UINT32 Cluster = Partition->BootSector.ExtendedBiosParamBlock.RootDirStart;
    UINT32 EntriesPerDir = ( Partition->BootSector.BiosParamBlock.ClusterSize << 9 ) / 32;
    
    UINT32 LastCluster = Cluster;

SearchDirectory:
    while((Status = Fat32ParsePath(Path, &Path, EntryName)) != 0) {
        int LastStatus = 0;
        while(Fat32ReadCluster(Partition, &Cluster, Partition->ReadCluster, &LastStatus) == FAT_SUCCESS) {
            FAT32_FILE_ENTRY* Entry = Fat32FindDirectoryClusterEntry(Partition, EntryName, Partition->ReadCluster, FindDirBuffer);
            if(!Entry) {
                LastCluster = Cluster;
                continue;
            }
            if(Status == 2) {
                if(FileEditParameters & FILE_EDIT_ATTRIBUTES) Entry->FileAttributes = NewAttributes;
                if(FileEditParameters & FILE_EDIT_CLUSTER) {
                    Entry->FirstClusterLow = ClusterFileSize;
                    Entry->FirstClusterHigh = ClusterFileSize >> 16;
                }
                if(FileEditParameters & FILE_EDIT_FILE_SIZE) {
                    Entry->FileSize = ClusterFileSize; // FILE_SIZE Set throw Cluster
                }
                
                Fat32WriteCluster(Partition, LastCluster, Partition->ReadCluster);
                return 1;
            } else {
                Cluster = (UINT32)Entry->FirstClusterLow | ((UINT32)Entry->FirstClusterHigh << 16);
                if(!Cluster) return 0;
                LastCluster = Cluster;
                goto SearchDirectory;
            }
        }
        return 0;
    }
    return 0;
}