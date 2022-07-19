#include <ddk.h>
#include <kernelruntime.h>
#include <ahci.h>
#include <drivectl.h>
#include <kintrinsic.h>

#define ATA_READ_DMA 0xC8 // limit of 0xff (255) Sectors
#define ATA_WRITE_DMA 0xCA
#define ATA_READ_DMA_EX 0x25 // Make use of Count High (16 Bit Count in FIS_H2D)
#define ATA_IDENTIFY_DEVICE_DMA 0xEE

#define AHCI_READWRITE_MAX_NUMBYTES ((DWORD)0xFFFF << 16)

typedef struct _AHCI_COMMAND_ADDRESS {
    DWORD NumBytes;
    void* BaseAddress;
} AHCI_COMMAND_ADDRESS;

void wr32(UINT32* Addr, UINT32 Val) {
    *Addr = Val;
}

DDKSTATUS AhciSataRead(RFDEVICE_OBJECT Device, UINT64 Address, UINT64 NumBytes, void* Buffer) {
    UINT64 Offset = Address & 0x1FF;
    UINT16 NumCommandAddresses = 1;
    AHCI_COMMAND_ADDRESS _CmdAddr[2] = {0}; // for less than AHCI_READWRITE_MAX_NUMBYTES
    AHCI_COMMAND_ADDRESS* CommandAddresses = _CmdAddr;
    UINT16 CmdIndex = 0;
    char Copy[0x200] = {0};

    UINT16 Count = NumBytes >> 9;
    if(NumBytes & 0x1FF) Count++;

    void* _InitialAddr = Buffer;

    

    if(NumBytes > AHCI_READWRITE_MAX_NUMBYTES) {
        // We will need more command addresses
        NumCommandAddresses = NumBytes / AHCI_READWRITE_MAX_NUMBYTES;
        if(NumCommandAddresses & (AHCI_READWRITE_MAX_NUMBYTES - 1)) NumCommandAddresses++;
        CommandAddresses = malloc(NumCommandAddresses * sizeof(AHCI_COMMAND_ADDRESS) + sizeof(AHCI_COMMAND_ADDRESS) /*1 Additionnal Command for unaligned address*/);
        if(!CommandAddresses) return KERNEL_SERR_UNSUFFICIENT_MEMORY;
        
    }
    UINT16 _PaddedBytes = 0x200 - Offset;
    if(Offset) {
        CommandAddresses[CmdIndex].NumBytes = _PaddedBytes;
        CommandAddresses[CmdIndex].BaseAddress = Copy;
        CmdIndex++;
        (char*)Buffer += _PaddedBytes;
        NumBytes -= _PaddedBytes;
    }

    for(UINT16 i = 0;i<NumCommandAddresses;i++) {
        if(NumBytes > AHCI_READWRITE_MAX_NUMBYTES) {
            CommandAddresses[CmdIndex].NumBytes = AHCI_READWRITE_MAX_NUMBYTES;
            NumBytes -= AHCI_READWRITE_MAX_NUMBYTES;
            (char*)Buffer += AHCI_READWRITE_MAX_NUMBYTES;
        }else {
            CommandAddresses[CmdIndex].NumBytes = NumBytes;
            NumBytes = 0;
        }
        CommandAddresses[CmdIndex].BaseAddress = Buffer;
        CmdIndex++;
    }

    if(Offset) NumCommandAddresses++;

    AhciHostToDevice(GetDeviceExtension(Device), (UINT64)_InitialAddr >> 9, ATA_READ_DMA, AHCI_DEVICE_LBA, Count, NumCommandAddresses, CommandAddresses);
    if(Offset) {
        memcpy(_InitialAddr, Copy + Offset, _PaddedBytes);
    }

    return KERNEL_SOK;
}

DDKSTATUS AhciSataWrite(RFDEVICE_OBJECT Device, UINT64 Address, UINT64 NumBytes, void* Buffer) {
    return KERNEL_SERR;
}

DDKSTATUS AhciAtapiRead(RFDEVICE_OBJECT Device, UINT64 Address, UINT64 NumBytes, void* Buffer) {
    return KERNEL_SERR;
}

DDKSTATUS AhciAtapiWrite(RFDEVICE_OBJECT Device, UINT64 Address, UINT64 NumBytes, void* Buffer) {
    return KERNEL_SERR;
}

void AhciInterruptHandler(RFDRIVER_OBJECT Driver, RFINTERRUPT_INFORMATION InterruptInformation) {

    RFAHCI_DEVICE Ahci = GetDeviceExtension(InterruptInformation->Device);
    
    UINT32 GlobalInterruptStatus = Ahci->Hba->InterruptStatus;
  
    if(!GlobalInterruptStatus) {
        return;
    }
    UINT32 HbaReceivedIntStatus = GlobalInterruptStatus;
    for(int i = 0;i<Ahci->NumPorts;i++) {
        if(GlobalInterruptStatus & 1) {
            HBA_PORT* HbaPort = &Ahci->HbaPorts[i];
            SystemDebugPrint(L"Interrupt on PORT %d", (UINT64)i);
            if(HbaPort->InterruptStatus.D2HRegisterFisInterrupt) {

                SystemDebugPrint(L"D2H_REG_FIS_INT");
                DWORD CmdIssue = HbaPort->CommandIssue;
                RFTHREAD* Pending = Ahci->Ports[i].PendingCommands;
                
                for(UINT64 c = 0;c<Ahci->MaxCommandSlots;c++, Pending++) {
                    if(*Pending && !(CmdIssue & 1)) {
                        SystemDebugPrint(L"Command#%d Compeleted", c);
                        RFTHREAD Th = *Pending;
                        *Pending = (RFTHREAD)-1;
                        IoFinish(Th);
                    }
                    CmdIssue >>= 1;
                }
                HbaPort->InterruptStatus.D2HRegisterFisInterrupt = 1;
            }
            if(HbaPort->InterruptStatus.SetDeviceBitsInterrupt) {
                SystemDebugPrint(L"SET_DEVICE_BITS");
            }
            if(HbaPort->InterruptStatus.DescriptorProcessed) SystemDebugPrint(L"DESCRIPTOR_PROCESSED");
            if(HbaPort->InterruptStatus.TaskFileErrorStatus) SystemDebugPrint(L"TASK_FILE_ERROR");
            if(HbaPort->InterruptStatus.UnknownFisInterrupt) SystemDebugPrint(L"UNKNOWN_FIS");
            // *(DWORD*)&HbaPort->InterruptEnable = *(DWORD*)&HbaPort->InterruptStatus;
            // *(DWORD*)&HbaPort->InterruptStatus = *(DWORD*)&HbaPort->InterruptStatus;
            HbaPort->CommandIssue = -1;
            HbaPort->SataActive = -1;
            *(UINT32*)&HbaPort->SataStatus = -1;
            wr32((UINT32*)&HbaPort->InterruptStatus, *(UINT32*)&HbaPort->InterruptStatus);
        }
        GlobalInterruptStatus >>= 1;
    }

    Ahci->Hba->InterruptStatus = HbaReceivedIntStatus;

}

DDKSTATUS DDKENTRY DriverEntry(RFDRIVER_OBJECT Driver){
    SystemDebugPrint(L"SATA AHCI Driver Startup : %d Controllers Detected", Driver->NumDevices);
    for(UINT i = 0;i<Driver->NumDevices;i++){
        RFDEVICE_OBJECT Device = Driver->Devices[i];
        SetDeviceFeature(Device, DEVICE_FORCE_MEMORY_ALLOCATION);
        BOOL MMio = FALSE;
        HBA_REGISTERS* Hba = PciGetBaseAddress(Device, 5, &MMio);
        KeMapMemory((void*)Hba, AHCI_CONFIGURATION_PAGES, PM_MAP | PM_CACHE_DISABLE | PM_WRITE_THROUGH);
        if(!MMio) {
            WriteDeviceLogA(Device, "Device BAR is not Memory Mapped (BAR5.IO = 1)");
            SetDeviceStatus(Device, KERNEL_SERR_INCORRECT_DEVICE_CONFIGURATION);
            continue;
        }

        
        Hba->GlobalHostControl.AhciEnable = 1;

        // Check if BIOS is device owner and take device ownership
        if(Hba->ExtendedHostCapabilities.BiosOsHandoff
        ){
            SystemDebugPrint(L"AHCI : Bios Owned");
            Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore = 1;
            while(!Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore);
            while(Hba->BiosOsHandoffControlAndStatus.BiosBusy);
            while(Hba->BiosOsHandoffControlAndStatus.BiosOwnedSemaphore);
            SystemDebugPrint(L"AHCI Ownership Taken : BIOS_OWNED : %x, OS_OWNED : %x", (UINT64)Hba->BiosOsHandoffControlAndStatus.BiosOwnedSemaphore, (UINT64)Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore);
        }
       


        RFAHCI_DEVICE Ahci = KeExtendedAlloc(KeGetCurrentThread(), ALIGN_VALUE(sizeof(AHCI_DEVICE), 0x1000), 0x1000, NULL, 0);
        if(!Ahci) return KERNEL_SERR_UNSUFFICIENT_MEMORY;
        KeMapMemory(Ahci, ALIGN_VALUE(sizeof(AHCI_DEVICE), 0x1000) >> 12, PM_MAP | PM_CACHE_DISABLE | PM_WRITE_THROUGH);
        ObjZeroMemory(Ahci);

        Ahci->Device = Device;
        Ahci->Hba = Hba;
        Ahci->HbaPorts = (HBA_PORT*)((char*)Hba + AHCI_PORTS_OFFSET);
        SetDeviceExtension(Device, Ahci);
        
         

        AhciReset(Ahci);

        if(Hba->HostCapabilities.x64AddressingCapability){
            SetDeviceFeature(Device, DEVICE_64BIT_ADDRESS_ALLOCATIONS);
            // SystemDebugPrint(L"AHCI Supports 64 Bit Memory Addresses.");
        }

        Ahci->MaxCommandSlots = Ahci->Hba->HostCapabilities.NumCommandSlots;
        

        


        
        Ahci->Hba->GlobalHostControl.InterruptEnable = 1;
        SetInterruptService(Device, AhciInterruptHandler);
        
    
        DWORD PortsImplemented = Ahci->Hba->PortsImplemented;
        UINT MaxPorts = Ahci->Hba->HostCapabilities.NumPorts + 1;
        for(UINT i = 0;i<MaxPorts;i++){
            if(PortsImplemented & 1){
                HBA_PORT* HbaPort = &Ahci->HbaPorts[i];
                KeMapMemory(HbaPort, 1, PM_MAP | PM_CACHE_DISABLE | PM_WRITE_THROUGH);
                AHCI_DEVICE_PORT* Port = &Ahci->Ports[Ahci->NumPorts];
                Ahci->NumPorts++;
                Port->Port = HbaPort;
                Port->PortIndex = i;
                Port->Controller = Device;
                Port->Ahci = Ahci;
                
                AhciInitializePort(Port);
                if(!Port->DeviceDetected) continue;
                SystemDebugPrint(L"SATA AHCI Device Detected. ATAPI = %x", HbaPort->CommandStatus.DeviceIsAtapi);
                if(HbaPort->CommandStatus.DeviceIsAtapi == 0 && HbaPort->SataStatus.DeviceDetection == 3) {
                    char* Sector0 = malloc(0x1000);
                    ZeroMemory(Sector0, 0x1000);
                    AHCI_COMMAND_ADDRESS CommandAddress = {0};
                    CommandAddress.BaseAddress = Sector0;
                    CommandAddress.NumBytes = 0x800;

                    KERNELSTATUS Status = AhciHostToDevice(Port, 0, ATA_READ_DMA, AHCI_DEVICE_LBA, CommandAddress.NumBytes >> 9, 1, &CommandAddress);
                    SystemDebugPrint(L"STATUS = %x, SECTOR0 (%x) : %s", Status, *(UINT64*)Sector0, Sector0);
                    SystemDebugPrint(L"SECTOR 1 (%x) : %s ||| SECTOR 2 (%x) : %s", *(UINT64*)(Sector0 + 0x200), Sector0 + 0x200, *(UINT64*)(Sector0 + 0x400), Sector0 + 0x400);
        
                    DRIVE_INFO* DriveInfo = malloc(sizeof(DRIVE_INFO));
                    ObjZeroMemory(DriveInfo);
                }
            }
            PortsImplemented >>= 1;
        }
        
        SystemDebugPrint(L"AHCI Controller Intialized.");

        

    }
    while(1);
}

void AhciReset(RFAHCI_DEVICE Ahci){
    // Turn off all ports
    {
        DWORD tmp = Ahci->Hba->PortsImplemented;
        for(int i = 0;i<0x20;i++) {
            if(tmp & 1) {
                Ahci->HbaPorts[i].CommandStatus.Start = 0;
                Ahci->HbaPorts[i].CommandStatus.FisReceiveEnable = 0;
                while(Ahci->HbaPorts[i].CommandStatus.CurrentCommandSlot
                || Ahci->HbaPorts[i].CommandStatus.FisReceiveRunning);
            }
            tmp >>= 1;
        }
    }

    
    Ahci->Hba->GlobalHostControl.HbaReset = 1;
    while(Ahci->Hba->GlobalHostControl.HbaReset);
    Ahci->Hba->GlobalHostControl.AhciEnable = 1;


}

#define CMD_LIST_ENTRIES 0x20

#define ATAPI_SIGNATURE 

void AhciInitializePort(RFAHCI_DEVICE_PORT Port){
    UINT64 NumBytes = ALIGN_VALUE(sizeof(AHCI_COMMAND_LIST_ENTRY) * Port->Ahci->MaxCommandSlots + 0x100 + sizeof(AHCI_COMMAND_TABLE) * Port->Ahci->MaxCommandSlots + 0x100, 0x1000);
    Port->AllocatedBuffer = AllocateDeviceMemory(Port->Controller, NumBytes, 0x1000);
    KeMapMemory(Port->AllocatedBuffer, NumBytes >> 12, PM_MAP | PM_CACHE_DISABLE | PM_WRITE_THROUGH);
    AHCI_COMMAND_LIST_ENTRY* CommandList = (void*)Port->AllocatedBuffer;
    AHCI_COMMAND_TABLE* CommandTables = (void*)(ALIGN_VALUE((UINT64)Port->AllocatedBuffer + sizeof(AHCI_COMMAND_LIST_ENTRY) * Port->Ahci->MaxCommandSlots, 0x100)); // Set after command list (struct is 128 bytes aligned)
    AHCI_RECEIVED_FIS* ReceivedFis = (void*)(Port->AllocatedBuffer + NumBytes - 0x100);
    
    Port->Port->CommandStatus.Start = 0;
    Port->Port->CommandStatus.FisReceiveEnable = 0;
    while(Port->Port->CommandStatus.CurrentCommandSlot || Port->Port->CommandStatus.FisReceiveRunning);

    Port->ReceivedFis = ReceivedFis;
    Port->CommandList = CommandList;
    Port->CommandTables = CommandTables;
    
    HBA_PORT* HbaPort = Port->Port;
    

    // if(*(DWORD*)&HbaPort->PortSignature == 0xEB140101) {
    //     HbaPort->CommandStatus.DeviceIsAtapi = 1;
    // }
    
    BOOL Atapi = HbaPort->CommandStatus.DeviceIsAtapi;
    for(UINT i = 0;i<CMD_LIST_ENTRIES;i++){
        CommandList[i].PrdtLength = NUM_PRDT_PER_CMDTBL;
        CommandList[i].Atapi = Atapi;
        CommandList[i].CommandTableAddress = (UINT64)&CommandTables[i];
    }
    HbaPort->FisBaseAddressLow = (UINT64)ReceivedFis;
    HbaPort->CommandListBaseAddressLow = (UINT64)CommandList;

    HbaPort->FisBaseAddressHigh = (UINT64)ReceivedFis >> 32;
    HbaPort->CommandListBaseAddressHigh = (UINT64)CommandList >> 32;

    
    Port->Port->InterruptEnable.D2HRegisterFisInterruptEnable = 1;
    Port->Port->InterruptEnable.SetDeviceBitsFisInterruptEnable = 1;
    Port->Port->InterruptEnable.DmaSetupFisInterruptEnable = 1;
    Port->Port->InterruptEnable.PortChangeInterruptEnable = 1;
    Port->Port->InterruptEnable.InterfaceNonFatalErrorEnable = 1;
    Port->Port->InterruptEnable.InterfaceFatalErrorEnable = 1;
    Port->Port->InterruptEnable.TaskFileErrorEnable = 1;


    *(DWORD*)&Port->Port->SataError = -1; // Clear SATA_ERROR


    Port->Port->SataControl.DeviceDetectionInitialization = 1;
    Sleep(5);
   
    DWORD Countdown = 12;
    BOOL DeviceDetected = 0;
    for(;;) {
        if(HbaPort->SataStatus.DeviceDetection == 3) {
            Port->Port->SataControl.DeviceDetectionInitialization = 0;
            DeviceDetected = 1;
            break;
        }

        if(!Countdown) break; // No device detection
        Sleep(1);
        Countdown--;
    }

    if(!DeviceDetected) {
        Port->DeviceDetected = 0;
        return;
    } else {
        Port->DeviceDetected = 1;
        SystemDebugPrint(L"ATTACHED_DEVICE_ON_PORT");
    }

    // while(HbaPort->TaskFileData.Busy || HbaPort->TaskFileData.DataTransferRequested);
    *(DWORD*)&Port->Port->SataError = -1; // Clear SATA_ERROR

    // BUFFERWRITE32(Port->Port->SataStatus, -1);

    // Wait for device initialization (transfer of initial FIS & Device Signature...)
    


    SystemDebugPrint(L"Port Signature : %x", HbaPort->PortSignature);


    // Enable Interrupts
    // Port->Port->CommandStatus.Start = 0; // Clear SATA_STATUS
    // BUFFERWRITE32(Port->Port->InterruptStatus, -1);


    Port->Port->CommandStatus.FisReceiveEnable = 1;
    Port->Port->CommandStatus.Start = 1;
    if(Port->Port->CommandStatus.ColdPresenceDetection) {
        Port->Port->CommandStatus.PowerOnDevice = 1;
    }

    Sleep(100);
}



int AhciIssueCommand(RFAHCI_DEVICE_PORT Port, UINT CommandIndex) {
    if(Port->PendingCommands[CommandIndex]) return -1; // Command slot is already in use
    
    Port->PendingCommands[CommandIndex] = KeGetCurrentThread();


    __SyncOr(&Port->Port->CommandIssue, CommandIndex);
    SystemDebugPrint(L"AHCI : COMMAND_READY");
    while(Port->Port->CommandIssue & (1 << CommandIndex));
    // Release Command Slot

    Port->PendingCommands[CommandIndex] = NULL;
    SystemDebugPrint(L"AHCI_COMMAND_ACK_RECEIVED.");
    return 0;
}

AHCI_COMMAND_TABLE* AhciAllocateCommand(RFAHCI_DEVICE_PORT Port, UINT16 PrdtCount, UINT8 FisSize, UINT8* CommandIndex){
    DWORD Mask = Port->Port->CommandIssue | Port->Port->SataActive;
    
    for(;;) {
        // Spin until a free command is found
        for(UINT i = 0;i<Port->Ahci->MaxCommandSlots;i++){
            if(!(Mask & 1)) {
                // It is a free command list
                AHCI_COMMAND_LIST_ENTRY* Entry = &Port->CommandList[i];
                ObjZeroMemory(Entry);
                Entry->PrdtLength = PrdtCount;
                Entry->CommandFisLength = FisSize >> 2;
            
                // Entry->Prefetchable = 1;
                // if(Port->Port->CommandStatus.DeviceIsAtapi) {
                //     Entry->Atapi = 1;
                // }
                AHCI_COMMAND_TABLE* Command = &Port->CommandTables[i];
                *CommandIndex = i;
                return Command;
            }
            Mask >>= 1;
        }
    }
}
KERNELSTATUS AhciHostToDevice(RFAHCI_DEVICE_PORT Port, UINT64 Lba, UINT8 Command, UINT8 Device, UINT16 Count, UINT16 NumCommandAddressDescriptors, AHCI_COMMAND_ADDRESS* CommandAddresses){    
    if(!Count) return KERNEL_SERR_INVALID_PARAMETER;
    AHCI_COMMAND_TABLE* CommandTable = NULL;
    UINT8 CommandIndex = 0;
    if(!(CommandTable = AhciAllocateCommand(Port, NumCommandAddressDescriptors, sizeof(ATA_FIS_H2D), &CommandIndex))) return KERNEL_SERR_OUT_OF_RANGE;

    ATA_FIS_H2D* Fis = (ATA_FIS_H2D*)CommandTable->CommandFis;
    Fis->FisType = FIS_TYPE_H2D;
    Fis->Command = Command;
    Fis->Count = Count;
    
    Fis->CommandControl = 1;
    Fis->Device = Device;
    Fis->Lba0 = Lba;
    Fis->Lba1 = Lba >> 16;
    Fis->Lba2 = Lba >> 24;
    Fis->Lba3 = Lba >> 40;



    for(UINT16 i = 0;i<NumCommandAddressDescriptors;i++) {
        // to avoid possible bugs
        PHYSICAL_REGION_DESCRIPTOR_TABLE* Prdt = &CommandTable->Prdt[i];
        Prdt->DataBaseAddress = (UINT64)CommandAddresses[i].BaseAddress;
        Prdt->DataByteCount = CommandAddresses[i].NumBytes - 1;
        Prdt->InterruptOnCompletion = 1;
        
    }
    
    return AhciIssueCommand(Port, CommandIndex);

}