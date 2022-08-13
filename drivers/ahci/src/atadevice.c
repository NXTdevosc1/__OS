#include <ddk.h>
#include <ahci.h>
int AhciInitAtaDevice(RFAHCI_DEVICE_PORT Port) {
    SystemDebugPrint(L"Initializing ATA Device");
    UINT64 TimerFrequency = GetHighPerformanceTimerFrequency();
    UINT64 Time = 0, PostTime = 0;

    char* Sector0 = malloc(0x1F400000);
    if(!Sector0) {
        SystemDebugPrint(L"ALLOCATION_FAILED.");
        while(1);
    }
    // SystemDebugPrint(L"MAX_LBA : %x , WWN : %s", IdentifyDevice.Max48BitLBA, IdentifyDevice.WorldWideName);
    ATA_IDENTIFY_DEVICE_DATA* IdentifyDeviceData = &Port->IdentifyDeviceData;
    UINT32 IdentifyCmd = AhciAllocateCommand(Port);
    AHCI_COMMAND_LIST_ENTRY* CmdEntry = &Port->CommandList[IdentifyCmd];
    CmdEntry->CommandFisLength = sizeof(ATA_FIS_H2D) >> 2;
    ATA_FIS_H2D* IdentifyH2d = (ATA_FIS_H2D*)Port->CommandTables[IdentifyCmd].CommandFis;
    IdentifyH2d->Command = ATA_IDENTIFY_DEVICE;
    IdentifyH2d->Device = AHCI_DEVICE_HOST;
    IdentifyH2d->FisType = FIS_TYPE_H2D;
    IdentifyH2d->Count = 0;
    IdentifyH2d->CommandControl = 1;
    Port->CommandTables[IdentifyCmd].Prdt[0].DataBaseAddress = (UINT64)IdentifyDeviceData;
    Port->CommandTables[IdentifyCmd].Prdt[0].DataByteCount = sizeof(ATA_IDENTIFY_DEVICE_DATA) - 1;

    Port->CommandList[IdentifyCmd].PrdtLength = 1;


    AhciIssueCommand(Port, IdentifyCmd);
    
    
    
    
    SystemDebugPrint(L"Identify : %ls, SECTORS : %x, IDD : %x", Port->DriveInfo.DriveName, IdentifyDeviceData->UserAddressableSectors, *(UINT64*)IdentifyDeviceData);
    
    SystemDebugPrint(L"REMOVABLE_MEDIA : %x, SECTORS_PER_TRACK : %x, EXUSER_SECTORS : %x", IdentifyDeviceData->CommandSetSupport.RemovableMediaFeature, IdentifyDeviceData->NumSectorsPerTrack, IdentifyDeviceData->AdditionalSupported.ExtendedUserAddressableSectorsSupported);
    
    if(IdentifyDeviceData->AdditionalSupported.ExtendedUserAddressableSectorsSupported) {
        Port->DriveInfo.MaxAddress = IdentifyDeviceData->ExtendedNumberOfUserAddressableSectors;
    } else {
        Port->DriveInfo.MaxAddress = IdentifyDeviceData->UserAddressableSectors;
    }
    SystemDebugPrint(L"%d MB, LSPS : %d", (Port->DriveInfo.MaxAddress << 9) / 0x100000, IdentifyDeviceData->PhysicalLogicalSectorSize.LogicalSectorsPerPhysicalSector);
    
    

    Time = GetHighPrecisionTimeSinceBoot();
    SystemDebugPrint(L"Reading Data... (500MB)");


    KERNELSTATUS Status = AhciSataRead(Port, 0, 0x1F400000, Sector0);
    
    PostTime = GetHighPrecisionTimeSinceBoot();
    SystemDebugPrint(L"Read Latency : %d ms", (PostTime - Time) * 1000 / TimerFrequency);
    SystemDebugPrint(L"STATUS = %x, SECTOR0 (%x)", Status, *(UINT64*)Sector0);
    SystemDebugPrint(L"SECTOR 1 (%x) : %s ||| SECTOR 2 (%x)", *(UINT64*)(Sector0 + 0x200), Sector0 + 0x200, *(UINT64*)(Sector0 + 0x400), Sector0 + 0x400);

    Port->Device = InstallDevice(
        Port->Ahci->Device,
        DEVICE_SOURCE_PARENT_CONTROLLER,
        DEVICE_TYPE_STORAGE,
        DEVICE_CLASS_ATA_DRIVE,
        DEVICE_SUBCLASS_AHCI_DRIVE,
        0,
        NULL
    );
    if(!Port->Device) {
        SystemDebugPrint(L"Failed to create device.");
        for(;;) Sleep(0x10000);
    }
    return 0;
}