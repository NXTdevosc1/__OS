#define __DLL_EXPORTS
#include <fat32.h>

// Return VALUE : ClusterStart = 0 : First Cluster, 0 if the function fails, otherwise : link clusters
UINT32 Fat32AllocateClusterChain(PFAT32_PARTITION Partition, UINT32 ClusterStart, UINT32 NumClusters) {
    if(!NumClusters) return 0;
    if(ClusterStart != 0 && ClusterStart < 2) return 0;
    if(ClusterStart > ((Partition->BootSector.DosParamBlock.TotalSectors - Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat * Partition->BootSector.BiosParamBlock.FatCount - Partition->BootSector.BiosParamBlock.ReservedAreaSize) / Partition->BootSector.BiosParamBlock.ClusterSize)) return 0; // Cluster start > Num Clusters
    if(Partition->FileSystemInformation.FreeClusters < NumClusters) return 0; // No Space
    UINT32 PrimaryFat = Partition->BootSector.DosParamBlock.HiddenSectors + Partition->BootSector.BiosParamBlock.ReservedAreaSize;
    UINT32 SecondaryFat = PrimaryFat + Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat;

    UINT32 ChainStart = 0;
    UINT32 AllocatedClusters = 0;

    UINT32 LastCluster = 0;
    UINT32 LastClusterIndexInSector = 0;
    UINT32 LastFatSector = 0;
    StartupInfo.Print("ALLOCATING_CLUSTERS...\n");

    __FS_BOOL PendingAllocation = 0;
    __FS_BOOL PendingWrite = 0;
    for(UINT32 x = 0;x<Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat;x++) {
        StartupInfo.Read(Partition->GeneralPurposeSector, PrimaryFat + x, 1);
        UINT32* Fat = (UINT32*)Partition->GeneralPurposeSector;

        for(UINT32 i = 0;i<0x80;i++, Fat++) {
            
            if(*Fat == 0) {
                if(LastCluster) {
                    if(PendingAllocation == 2) {
                        UINT32* Entry = &((UINT32*)Partition->SecondaryPurposeSector)[LastClusterIndexInSector];
                        *Entry = (x << 7) + i;
                        StartupInfo.Write(Partition->SecondaryPurposeSector, PrimaryFat + LastFatSector, 1);
                        StartupInfo.Write(Partition->SecondaryPurposeSector, SecondaryFat + LastFatSector, 1);
                    }else {
                        ((UINT32*)Partition->GeneralPurposeSector)[LastClusterIndexInSector] = (x << 7) + i;
                    }
                    
                }else {
                    ChainStart = (x << 7) + i;
                }
                        PendingWrite = 1;
                LastCluster = (x << 7) + i;
                LastClusterIndexInSector = i;
                LastFatSector = x;
                PendingAllocation = 1;
                AllocatedClusters++;
                if(AllocatedClusters == NumClusters) {
                    *Fat = FAT32_ENDOF_CHAIN;
                    Partition->FileSystemInformation.RecentlyAllocatedCluster = LastCluster;
                    // Pending Write
                    StartupInfo.Write(Partition->GeneralPurposeSector, PrimaryFat + x, 1);
                    StartupInfo.Write(Partition->GeneralPurposeSector, SecondaryFat + x, 1);
                    goto Finish;
                }
            }
        }
            // Do not write if pending allocation
        if(PendingAllocation == 1) {
            PendingAllocation = 2; // Allocate from different sector
            Fat32MemCopy(Partition->SecondaryPurposeSector, Partition->GeneralPurposeSector, 0x200);
        }
        if(PendingWrite) {
            StartupInfo.Write(Partition->GeneralPurposeSector, PrimaryFat + x, 1);
            StartupInfo.Write(Partition->GeneralPurposeSector, SecondaryFat + x, 1);
            PendingWrite = 0;
        }
    }
Finish:
    StartupInfo.Print("ALLOCATING_FINISH... (CHAIN_START : %d)\n", ChainStart);

    if(ClusterStart) {
        // List Last Cluster to the previous one
        UINT32 Cluster = ClusterStart;
        UINT32 FatSector = (Cluster >> 7);
        // Retreive last entry
        for(;;) {
            if(FatSector != LastFatSector) {
                StartupInfo.Read(Partition->GeneralPurposeSector, PrimaryFat + FatSector, 1);
                LastFatSector = FatSector;
            }
            UINT32 FatEntry = Cluster & 0x7F;
            UINT32* Fat = (UINT32*)Partition->GeneralPurposeSector + FatEntry;
        
            if(*Fat == FAT32_ENDOF_CHAIN) {
                // Write Last Chain
                *Fat = ChainStart;
                StartupInfo.Write(Partition->GeneralPurposeSector, PrimaryFat + FatSector, 1);
                StartupInfo.Write(Partition->GeneralPurposeSector, SecondaryFat + FatSector, 1);
                break;
            }else {
                Cluster = *Fat;
                FatSector = (Cluster >> 7);
            }
        }
    }
    // Sub FAT32_FS : Free Clusters
    Partition->FileSystemInformation.FreeClusters-=NumClusters;
    StartupInfo.Write(&Partition->FileSystemInformation, Partition->BootSector.DosParamBlock.HiddenSectors + Partition->BootSector.ExtendedBiosParamBlock.FsInfoLba, 1);
    StartupInfo.Write(&Partition->FileSystemInformation, Partition->SecondaryFsInfo, 1);

    // Reset FAT Cache
    Partition->FatSectorNumber = 0;

    return ChainStart;
}

UINT32 FSBASEAPI Fat32GetLastCluster(PFAT32_PARTITION Partition, UINT32 ClusterChainStart) {
    UINT32 Cluster = ClusterChainStart;
    UINT32 LastFatSector = 0xFFFFFFFF;
    UINT32 FatSector = Cluster >> 7;
    UINT32 PrimaryFat = Partition->BootSector.DosParamBlock.HiddenSectors + Partition->BootSector.BiosParamBlock.ReservedAreaSize;
    // Retreive last entry
    for(;;) {
        if(FatSector != LastFatSector) {
            StartupInfo.Read(Partition->GeneralPurposeSector, PrimaryFat + FatSector, 1);
            LastFatSector = FatSector;
        }
        UINT32 FatEntry = Cluster & 0x7F;
        UINT32* Fat = &(((UINT32*)Partition->GeneralPurposeSector)[FatEntry]);
        if(*Fat == FAT32_ENDOF_CHAIN) {
            return Cluster;
        }else {
            Cluster = *Fat;
            FatSector = Cluster >> 7;
        }
    }
}
__FS_BOOL Fat32MarkBadClusterChain(PFAT32_PARTITION Partition, UINT32 ClusterStart) {
    return 0;
}
__FS_BOOL Fat32FreeClusterChain(PFAT32_PARTITION Partition, UINT32 ClusterStart) {
    return 0;
}

UINT32 Fat32RetreiveClusterCount(PFAT32_PARTITION Partition, UINT32 ClusterStart) {
    return 0;
}

__FS_BOOL Fat32UnmarkBadClusterChain(PFAT32_PARTITION Partition, UINT32 ClusterStart){
    return 0;
}

int Fat32ReadCluster(PFAT32_PARTITION Partition, UINT32* _Cluster, void* Buffer, int* LastCode /*set to 0*/) {
    UINT32 Status = *LastCode;
    if(Status == FAT_END) return FAT_END;
    UINT32 Cluster = *_Cluster;
    if(Cluster < 2) return FAT_INVALID_PARAMETER;

    UINT32 FatSector = Partition->BootSector.DosParamBlock.HiddenSectors + Partition->BootSector.BiosParamBlock.ReservedAreaSize + (Cluster >> 7);
    UINT32 ClusterIndex = Cluster & 0x7F;

    UINT32* Fat = Partition->FatSector;
    if(Partition->FatSectorNumber != FatSector) {
        StartupInfo.Read(Fat, FatSector, 1);
        Partition->FatSectorNumber = FatSector;
    }

    UINT32 ClusterData = Fat[ClusterIndex];
    if(ClusterData == FAT32_ENDOF_CHAIN) *LastCode = FAT_END;
    else *_Cluster = ClusterData; // Point to the next cluster
    StartupInfo.Read(Buffer, Partition->BootSector.DosParamBlock.HiddenSectors + Partition->BootSector.BiosParamBlock.ReservedAreaSize + Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat * Partition->BootSector.BiosParamBlock.FatCount + Partition->BootSector.BiosParamBlock.ClusterSize * (Cluster - 2), Partition->BootSector.BiosParamBlock.ClusterSize);
    return FAT_SUCCESS;
}
__FS_BOOL Fat32WriteCluster(PFAT32_PARTITION Partition, UINT32 Cluster, void* Buffer) {
    if(Cluster < 2) return 0;

    return StartupInfo.Write(Buffer, Partition->BootSector.DosParamBlock.HiddenSectors + Partition->BootSector.BiosParamBlock.ReservedAreaSize + Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat * Partition->BootSector.BiosParamBlock.FatCount + Partition->BootSector.BiosParamBlock.ClusterSize * (Cluster - 2), Partition->BootSector.BiosParamBlock.ClusterSize);
}

UINT32 FSBASEAPI Fat32GetNextCluster(PFAT32_PARTITION Partition, UINT32 LastCluster) {
    if(LastCluster == FAT32_ENDOF_CHAIN) return 0;
    UINT32 FatSector = Partition->BootSector.DosParamBlock.HiddenSectors + Partition->BootSector.BiosParamBlock.ReservedAreaSize + (LastCluster >> 7);
    UINT32 ClusterIndex = LastCluster & 0x7F;
    if(Partition->FatSectorNumber != FatSector) {
        StartupInfo.Read(Partition->FatSector, FatSector, 1);
        Partition->FatSectorNumber = FatSector;
    }

    return Partition->FatSector[ClusterIndex];
}