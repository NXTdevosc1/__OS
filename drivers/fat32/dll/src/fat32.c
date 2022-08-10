#define __DLL_EXPORTS
#include <fat32.h>

int Fat32DriverInitialized = 0;

FAT32_DRIVER_INIT_STRUCTURE StartupInfo = {0};

// Only for WINDOWS, the dll does not require an EntryPoint
unsigned int __stdcall DllMain(
    void* hInstDll, unsigned int fdwReason, void* lpReserved
) {
    return 1;
}

void Fat32MemCopy(void* Destination, const void* Source, unsigned long long NumBytes) {
    if(FAT32_ALIGN_CHECK(NumBytes, 8)) {
        unsigned long long* ConvertedDest = Destination;
        const unsigned long long* ConvertedSource = Source;
        NumBytes >>= 3;
        for(unsigned long long i = 0;i<NumBytes;i++, ConvertedDest++, ConvertedSource++) {
            *ConvertedDest = *ConvertedSource;
        }
    } else if(FAT32_ALIGN_CHECK(NumBytes, 4)) {
        unsigned long int* ConvertedDest = Destination;
        const unsigned long int* ConvertedSource = Source;
        NumBytes >>= 2;
        for(unsigned long long i = 0;i<NumBytes;i++, ConvertedDest++, ConvertedSource++) {
            *ConvertedDest = *ConvertedSource;
        }
    } else if(FAT32_ALIGN_CHECK(NumBytes, 2)) {
        unsigned short* ConvertedDest = Destination;
        const unsigned short* ConvertedSource = Source;
        NumBytes >>= 1;
        for(unsigned long long i = 0;i<NumBytes;i++, ConvertedDest++, ConvertedSource++) {
            *ConvertedDest = *ConvertedSource;
        }
    } else {
        unsigned char* ConvertedDest = Destination;
        const unsigned char* ConvertedSource = Source;
        for(unsigned long long i = 0;i<NumBytes;i++, ConvertedDest++, ConvertedSource++) {
            *ConvertedDest = *ConvertedSource;
        }
    }
}

void Fat32MemClear(void* Destination, unsigned long long NumBytes) {
    char* d = Destination;
    for(unsigned long long i = 0;i<NumBytes;i++, d++) {
        *d = 0;
    }
    return;
    if(FAT32_ALIGN_CHECK(NumBytes, 8)) {
        unsigned long long* ConvertedDest = Destination;
        NumBytes >>= 3;
        for(unsigned long long i = 0;i<NumBytes;i++, ConvertedDest++) {
            *ConvertedDest = 0;
        }
    } else if(FAT32_ALIGN_CHECK(NumBytes, 4)) {
        unsigned long int* ConvertedDest = Destination;
        NumBytes >>= 2;
        for(unsigned long long i = 0;i<NumBytes;i++, ConvertedDest++) {
            *ConvertedDest = 0;
        }
    } else if(FAT32_ALIGN_CHECK(NumBytes, 2)) {
        unsigned short* ConvertedDest = Destination;
        NumBytes >>= 1;
        for(unsigned long long i = 0;i<NumBytes;i++, ConvertedDest++) {
            *ConvertedDest = 0;
        }
    } else {
        unsigned char* ConvertedDest = Destination;
        for(unsigned long long i = 0;i<NumBytes;i++, ConvertedDest++) {
            *ConvertedDest = 0;
        }
    }
}

UINT16 Fat32StrlenA(const UINT8* str) {
    UINT16 Len = 0;
    while(*str) {
        Len++;
        str++;
    }
    return Len;
}

UINT16 Fat32StrlenW(const UINT16* str) {
    UINT16 Len = 0;
    while(*str) {
        Len++;
        str++;
    }
    return Len;
}


__FS_BOOL FSBASEAPI Fat32DriverInit(FAT32_DRIVER_INIT_STRUCTURE* InitStructure){
    if(Fat32DriverInitialized) return 0;
    Fat32MemCopy(&StartupInfo, InitStructure, sizeof(FAT32_DRIVER_INIT_STRUCTURE));
    if(InitStructure->Debug) {
        InitStructure->Print("FAT32 Driver Initialized. %d\n", 123);
    }
    Fat32DriverInitialized = 1;
    return 1;
}

UINT8 OemName[8] = "__DEV_OS";
UINT8 Fat32FsLabel[8] = "FAT32   ";

UINT8 FsInfoSig0[4] = "RRaA";
UINT8 FsInfoSig1[4] = "rrAa";


PFAT32_PARTITION FSBASEAPI Fat32CreatePartition(UINT32 BaseLba, UINT32 NumClusters, UINT8 ClusterSize, UINT8* VolumeLabel, UINT16 NumReservedSectors, char* BootSector){
    PFAT32_PARTITION Partition = StartupInfo.AllocateMemory(sizeof(FAT32_PARTITION));
    Fat32MemClear(Partition, sizeof(FAT32_PARTITION));

    
    Partition->ReadCluster = StartupInfo.AllocateMemory(ClusterSize << 9);
    if(!Partition->ReadCluster) return (void*)0;

    Fat32MemCopy(&Partition->BootSector, BootSector, 0x200);

    // // x86 Code for instruction : jmp 0x7C5A
    // Partition->BootSector.Jmp[0] = 0xEB;
    // Partition->BootSector.Jmp[1] = 0x58;
    // // SINCE DOS 2.0, A valid boot sector must start with jmp (0xEB ? 0x90)
    // // followed by a NOP instruction (0x90)
    // Partition->BootSector.Jmp[2] = 0x90; //sizeof(FAT32_BOOTSECTOR) - 442; // Boot Code Offset
    

    Fat32MemCopy(Partition->BootSector.OemName, OemName, 8);
    // BIOS_PARAMETER_BLOCK
    Partition->BootSector.BiosParamBlock.SectorSize = 0x200;
    Partition->BootSector.BiosParamBlock.ClusterSize = ClusterSize;
    Partition->BootSector.BiosParamBlock.FatCount = 2;
    Partition->BootSector.BiosParamBlock.MediaDescriptor = 0xF8; // Fixed disk/Removable media
    Partition->BootSector.BiosParamBlock.ReservedAreaSize = NumReservedSectors + 0x10; // BOOT_SECTOR + BOOT_SECTOR_COPY + FSINFO
    
    UINT32 SectorsPerFat = (((UINT64)NumClusters + 2) << 2) / 0x200; /*2 Reserved entries + 128 Entry per sector (>>7) / 512 */ 
    if((((UINT64)NumClusters + 2) << 2) & (0x1FF)) SectorsPerFat++;

    UINT32 TotalSectors = NumClusters * Partition->BootSector.BiosParamBlock.ClusterSize + Partition->BootSector.BiosParamBlock.ReservedAreaSize + SectorsPerFat * Partition->BootSector.BiosParamBlock.FatCount;

    // DOS3_PARAMETER_BLOCK
    Partition->BootSector.DosParamBlock.HiddenSectors = BaseLba;
    Partition->BootSector.DosParamBlock.NumHeads = 0xFF00;
    Partition->BootSector.DosParamBlock.SectorsPerTrack = 63;
    Partition->BootSector.DosParamBlock.TotalSectors = TotalSectors;
    
    // FAT32_EXTENDED_BIOS_PARAMETER_BLOCK
    Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat = SectorsPerFat;
    Partition->BootSector.ExtendedBiosParamBlock.RootDirStart = 2;
    Partition->BootSector.ExtendedBiosParamBlock.FsInfoLba = NumReservedSectors + 1;
    Partition->BootSector.ExtendedBiosParamBlock.BootSectorCopyLba = NumReservedSectors + 3;
    Partition->BootSector.ExtendedBiosParamBlock.ExtendedBootSignature = 0x29;
    Partition->BootSector.ExtendedBiosParamBlock.PhysicalDriveNumber = 0x80;

    UINT32 SecondaryFsInfoLba = NumReservedSectors + 4;

    Partition->SecondaryFsInfo = SecondaryFsInfoLba;

    UINT16 LenVolumeLabel = Fat32StrlenA(VolumeLabel);
    for(int i = 0;i<11;i++) {
        if(i >= LenVolumeLabel) Partition->BootSector.ExtendedBiosParamBlock.VolumeLabel[i] = 0x20;
        else Partition->BootSector.ExtendedBiosParamBlock.VolumeLabel[i] = VolumeLabel[i];
    }
    Fat32MemCopy(Partition->BootSector.ExtendedBiosParamBlock.FsLabel, Fat32FsLabel, 8);

    // BOOT_SIGNATURE

    Partition->BootSector.BootSignature = 0xAA55;

    // FS_INFO
    Fat32MemCopy(Partition->FileSystemInformation.Signature0, FsInfoSig0, 4);
    Fat32MemCopy(Partition->FileSystemInformation.Signature1, FsInfoSig1, 4);
    Partition->FileSystemInformation.FreeClusters = NumClusters - 1; // Root Dir Cluster
    Partition->FileSystemInformation.RecentlyAllocatedCluster = 0;
    Partition->FileSystemInformation.BootSignature = 0xAA55;
    Partition->FileSystemInformation.BootSignature2 = 0xAA55;
    
    Partition->RootDirLba = Partition->BootSector.DosParamBlock.HiddenSectors + Partition->BootSector.BiosParamBlock.ReservedAreaSize + Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat * Partition->BootSector.BiosParamBlock.FatCount;


    if(!StartupInfo.Write(&Partition->BootSector, BaseLba, 1)) return (void*)0;
    // Setup Secondary BOOT_SECTOR
    Fat32MemCopy(Partition->GeneralPurposeSector, &Partition->BootSector, 0x200);
    FAT32_BOOTSECTOR* SecondBootSector = (FAT32_BOOTSECTOR*)Partition->GeneralPurposeSector;
    SecondBootSector->ExtendedBiosParamBlock.FsInfoLba = SecondaryFsInfoLba;
    SecondBootSector->ExtendedBiosParamBlock.BootSectorCopyLba = 0;
    if(!StartupInfo.Write(SecondBootSector, BaseLba + Partition->BootSector.ExtendedBiosParamBlock.BootSectorCopyLba, 1)) return (void*)0;
    if(!StartupInfo.Write(&Partition->FileSystemInformation, BaseLba + Partition->BootSector.ExtendedBiosParamBlock.FsInfoLba, 2)) return (void*)0;
    if(!StartupInfo.Write(&Partition->FileSystemInformation, BaseLba + SecondBootSector->ExtendedBiosParamBlock.FsInfoLba, 2)) return (void*)0;
    Fat32MemClear(Partition->GeneralPurposeSector, 0x200);

    UINT32* Fat = (UINT32*)Partition->GeneralPurposeSector;
    Fat[0] = 0xFFFFFF00 | (Partition->BootSector.BiosParamBlock.MediaDescriptor); // reserved cluster
    Fat[1] = 0xFFFFFFFF; // reserved cluster
    Fat[2] = 0x0FFFFFFF; // Root Dir Cluster

    // Format primary & secondary FAT

    // Write primary FAT
    if(!StartupInfo.Write(&Partition->GeneralPurposeSector, BaseLba + Partition->BootSector.BiosParamBlock.ReservedAreaSize, 1)) return (void*)0;
    // Write secondary FAT
    if(!StartupInfo.Write(&Partition->GeneralPurposeSector, BaseLba + Partition->BootSector.BiosParamBlock.ReservedAreaSize + Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat, 1)) return (void*)0;
    
    // Clear Primary & Secondary FAT
    Fat32MemClear(Partition->GeneralPurposeSector, 0x200);
    for(UINT32 i = 1;i<Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat;i++) {
        if(!StartupInfo.Write(&Partition->GeneralPurposeSector, BaseLba + Partition->BootSector.BiosParamBlock.ReservedAreaSize + i, 1)) return (void*)0;
        if(!StartupInfo.Write(&Partition->GeneralPurposeSector, BaseLba + Partition->BootSector.BiosParamBlock.ReservedAreaSize + Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat + i, 1)) return (void*)0;
    }

    // Clear Root Directory
    Fat32MemClear(Partition->ReadCluster, Partition->BootSector.BiosParamBlock.ClusterSize << 9);
    // Manually create volume label (SHORT FILE NAME (MAX 11 CHARACTERS))
    FAT32_FILE_ENTRY* VolumeNameDirEntry = (FAT32_FILE_ENTRY*)Partition->ReadCluster;
    VolumeNameDirEntry->FileAttributes = 0x08;
    /*Copy Name*/
    UINT8* TmpVlabel = VolumeLabel;
    for(UINT16 i = 0;i<11;i++) {
        if(*TmpVlabel) {
            UINT8 c = *TmpVlabel;
            // if(c >= 'a' && c <= 'z') c-=32; // Convert to uppercase
            VolumeNameDirEntry->ShortFileName[i] = *TmpVlabel;
            TmpVlabel++;
        } else {
            VolumeNameDirEntry->ShortFileExtension[i] = 0x20;
        }
    }
    
    if(!StartupInfo.Write(Partition->ReadCluster, BaseLba + Partition->BootSector.BiosParamBlock.ReservedAreaSize + Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat * Partition->BootSector.BiosParamBlock.FatCount, ClusterSize)) return (void*)0;
    if(StartupInfo.Debug) {
        StartupInfo.Print("PARTITION CREATED : BASE_LBA : %I64x, BS_COPY : %I64x, FS_INFO : %I64x, FAT_SIZE : %I64x\n", (UINT64)BaseLba, (UINT64)BaseLba + Partition->BootSector.ExtendedBiosParamBlock.BootSectorCopyLba, (UINT64)BaseLba + Partition->BootSector.ExtendedBiosParamBlock.FsInfoLba, (UINT64)Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat);
        UINT64 FirstFat = BaseLba + Partition->BootSector.BiosParamBlock.ReservedAreaSize;
        UINT64 SecondFat = BaseLba + Partition->BootSector.BiosParamBlock.ReservedAreaSize + Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat;
        UINT64 RootDir = BaseLba + Partition->BootSector.BiosParamBlock.ReservedAreaSize + Partition->BootSector.ExtendedBiosParamBlock.SectorsPerFat * Partition->BootSector.BiosParamBlock.FatCount;

        StartupInfo.Print("FIRST_FAT : %llu, SECOND_FAT : %llu, ROOT_DIRECTORY : %llu\n", FirstFat, SecondFat, RootDir);
    }

    // Creating Essential Files
    UINT16 WVolumeLabel[100] = {0};
    for(UINT16 i = 0;i<LenVolumeLabel;i++) {
        if(i == 100) break;
        WVolumeLabel[i] = VolumeLabel[i];
    }
   
    return Partition;
}

__FS_BOOL FSBASEAPI Fat32WriteReservedArea(PFAT32_PARTITION Partition, UINT16 Sector, UINT16 NumSectors, void* Buffer) {
    if(Sector + NumSectors > Partition->BootSector.BiosParamBlock.ReservedAreaSize - 3) return 0;
    return StartupInfo.Write(Buffer, Partition->BootSector.DosParamBlock.HiddenSectors + Sector + 1, NumSectors);
}

int Fat32ParsePath(UINT16* Path, UINT16** AdvancePath, UINT16* Entry /*stored in lowercase*/){
    while(*Path == L'\\' || *Path == L'/') Path++;
    if(*Path == 0) return 0;
    while(*Path != L'\\' && *Path != L'/') {
        if(*Path == 0) {
            *Entry = 0;
            *AdvancePath = Path;
            return 2; // last entry
        }
        *Entry = *Path;
        Entry++;
        Path++;
    }
    *Entry = 0;
    while(*Path == L'\\' || *Path == L'/') Path++;
    if(*Path == 0) {
        *AdvancePath = Path;
        return 2; // last entry
    }
    *AdvancePath = Path;
    return 1;
}