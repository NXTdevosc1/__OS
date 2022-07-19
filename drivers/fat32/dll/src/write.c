#define __DLL_EXPORTS
#include <fat32.h>

int FSBASEAPI Fat32WriteFile(PFAT32_PARTITION Partition, UINT16* Path, void* _Buffer, UINT32 Position, UINT32 NumBytes) {
    if(!NumBytes) return FAT_INVALID_PARAMETER;
    FAT_FILE_INFORMATION FileInformation = {0};
    if(!Fat32GetFileInformation(Partition, Path, &FileInformation)) return FAT_NOT_FOUND;
    // Append to File
    UINT32 ClusterSize = Partition->BootSector.BiosParamBlock.ClusterSize << 9;
    if(Position >= FileInformation.FileSize) {
        UINT32 RequiredClusters = ((Position + NumBytes) - FileInformation.FileSize) / (Partition->BootSector.BiosParamBlock.ClusterSize << 9);
        if(((Position + NumBytes) - FileInformation.FileSize) % (Partition->BootSector.BiosParamBlock.ClusterSize << 9)) RequiredClusters++;
        
        // Allocate necessary clusters
        UINT32 NextCluster = Fat32AllocateClusterChain(Partition, FileInformation.ClusterStart, RequiredClusters);
        if(!NextCluster) return FAT_NO_ENOUGH_SPACE;
        if(!FileInformation.ClusterStart) {
            FileInformation.ClusterStart = NextCluster;
            Fat32EditFile(Partition, Path, FILE_EDIT_CLUSTER, 0, NextCluster, 0);
        }



        UINT32 Cluster = NextCluster;
        UINT32 BlankSize = Position - FileInformation.FileSize;
        
        FileInformation.FileSize = Position + NumBytes;
        Fat32EditFile(Partition, Path, FILE_EDIT_FILE_SIZE, 0, FileInformation.FileSize, 0);
        
        // Set blank cluster
        if(BlankSize) Fat32MemClear(Partition->ReadCluster, ClusterSize);
        UINT32 RemainingBytes = NumBytes;
        char* Buffer = _Buffer;
        while((NextCluster = Fat32GetNextCluster(Partition, NextCluster))) {
            if(BlankSize) {
                if(BlankSize < ClusterSize) {
                    UINT32 BufferPosition = Position % ClusterSize;
                    UINT32 CopySize = ClusterSize - BufferPosition;
                    if(RemainingBytes < CopySize) CopySize = RemainingBytes;
                    Fat32MemCopy((char*)Partition->ReadCluster + BufferPosition, Buffer, CopySize);
                    RemainingBytes -= CopySize;
                    Buffer += CopySize;
                    BlankSize = 0;
                    Fat32WriteCluster(Partition, Cluster, Partition->ReadCluster);

                }else {
                    Fat32WriteCluster(Partition, Cluster, Partition->ReadCluster);
                    BlankSize -= ClusterSize;
                }
            }else {
                if(RemainingBytes < ClusterSize) {
                    Fat32MemCopy(Partition->ReadCluster, Buffer, RemainingBytes);
                    Fat32MemClear((char*)Partition->ReadCluster + RemainingBytes, ClusterSize - RemainingBytes);
                    Buffer += RemainingBytes;
                    RemainingBytes = 0;
                    Fat32WriteCluster(Partition, Cluster, Partition->ReadCluster);
                }else {
                    Fat32WriteCluster(Partition, Cluster, Buffer);
                    Buffer+=ClusterSize;
                    RemainingBytes-=ClusterSize;
                }
            }
            Cluster = NextCluster;
            if(!RemainingBytes) break;
        }
        return FAT_SUCCESS;
    }
// Modify file content & (Maybe) Append to file
    
    // Check if there is clusters to be appended 
    if(Position + NumBytes > FileInformation.FileSize) {
        UINT32 RequiredClusters = ((Position + NumBytes) - FileInformation.FileSize) / ClusterSize;
        if(((Position + NumBytes) - FileInformation.FileSize) % ClusterSize) RequiredClusters++;
        UINT32 NewClusters = Fat32AllocateClusterChain(Partition, FileInformation.ClusterStart, RequiredClusters);
        if(!NewClusters) return FAT_NO_ENOUGH_SPACE;

        FileInformation.FileSize = Position + NumBytes;
        Fat32EditFile(Partition, Path, FILE_EDIT_FILE_SIZE, 0, FileInformation.FileSize, 0);
    }

    UINT32 TmpPos = Position;
    UINT32 NextCluster = FileInformation.ClusterStart;
    UINT32 Cluster = NextCluster;
    UINT32 RemainingBytes = NumBytes;
    char* Buffer = _Buffer;
    while((NextCluster = Fat32GetNextCluster(Partition, NextCluster))) {
        if(!TmpPos) {
            if(RemainingBytes >= ClusterSize) {
                Fat32WriteCluster(Partition, Cluster, Buffer);
                Buffer += ClusterSize;
                RemainingBytes -= ClusterSize;
            } else {
                UINT32 Cl = Cluster;
                int LastCode = 0;
                Fat32ReadCluster(Partition, &Cl, Partition->ReadCluster, &LastCode);
                Fat32MemCopy(Partition->ReadCluster, Buffer, RemainingBytes);
                Buffer += RemainingBytes;
                RemainingBytes = 0;
                Fat32WriteCluster(Partition, Cluster, Partition->ReadCluster);
            }
        }
        else if(TmpPos < ClusterSize) {
            // write first data
            if(RemainingBytes >= ClusterSize - TmpPos) {
                UINT32 Cl = Cluster;
                int LastCode = 0;
                if(TmpPos > 0) Fat32ReadCluster(Partition, &Cl, Partition->ReadCluster, &LastCode);

                Fat32MemCopy((char*)Partition->ReadCluster + TmpPos, Buffer, ClusterSize - TmpPos);
                Buffer += ClusterSize - TmpPos;
                RemainingBytes -= ClusterSize - TmpPos;
                Fat32WriteCluster(Partition, Cluster, Partition->ReadCluster);
            }else {
                UINT32 Cl = Cluster;
                int LastCode = 0;
                if(TmpPos > 0) Fat32ReadCluster(Partition, &Cl, Partition->ReadCluster, &LastCode);
                Fat32MemCopy((char*)Partition->ReadCluster + TmpPos, Buffer, RemainingBytes);
                Buffer += RemainingBytes;
                RemainingBytes = 0;
                Fat32WriteCluster(Partition, Cluster, Partition->ReadCluster);
            }

            TmpPos = 0;
        } else {
            TmpPos -= ClusterSize;
        }
        Cluster = NextCluster;
        if(!RemainingBytes) break;
    }

    return 1;
}