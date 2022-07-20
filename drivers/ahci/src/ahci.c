#include <ddk.h>
#include <kernelruntime.h>
#include <ahci.h>
#include <drivectl.h>
#include <kintrinsic.h>
#include <assemblydef.h>

#define ATA_READ_DMA 0xC8 // limit of 0xff (255) Sectors
#define ATA_WRITE_DMA 0xCA
#define ATA_READ_DMA_EX 0x25 // Make use of Count High (16 Bit Count in FIS_H2D)
#define ATA_IDENTIFY_DEVICE_DMA 0xEE

#define AHCI_READWRITE_MAX_NUMBYTES ((DWORD)0xFFFF << 16)

typedef struct _AHCI_COMMAND_ADDRESS {
    DWORD NumBytes;
    void* BaseAddress;
} AHCI_COMMAND_ADDRESS;

void wr32(void* Addr, UINT32 Val) {
    *(UINT32*)Addr = Val;
}

DDKSTATUS AhciSataRead(RFAHCI_DEVICE_PORT Port, UINT64 Address, UINT64 NumBytes, void* _Buffer) {
    char* Buffer = _Buffer;
    UINT64 Remaining = NumBytes;
    RFTHREAD Thread = KeGetCurrentThread();
    while(Remaining) {
        RFTHREAD* Th = Port->PendingCommands;
        for(UINT i = 0;i<Port->Ahci->MaxCommandSlots;i++, Th++) {
            // Clear all acked reads
            if(*Th == Thread && (Port->DoneCommands & (1 << i))) {
                // Release Command Slot
                SystemDebugPrint(L"SATA_READ : Command Done");
                *Th = NULL;
                Port->DoneCommands &= ~(1 << i);
                Port->UsedCommandSlots &= ~(1 << i);
            }
        }
        UINT64 ReadBytes = Remaining;
        if(Remaining > MAX_PRDT_BYTE_COUNT) {
            ReadBytes = MAX_PRDT_BYTE_COUNT;
            Remaining -= MAX_PRDT_BYTE_COUNT;
        } else {
            Remaining = 0;
        }

        UINT32 CommandSlot = AhciAllocateCommand(Port);
        AHCI_COMMAND_LIST_ENTRY* CmdEntry = &Port->CommandList[CommandSlot];
        AHCI_COMMAND_TABLE* CmdTbl = &Port->CommandTables[CommandSlot];
        CmdEntry->CommandFisLength = sizeof(ATA_FIS_H2D) >> 2;
        CmdEntry->PrdtByteCount = ReadBytes >> 9;
        CmdEntry->PrdtLength = 1;
        ATA_FIS_H2D* H2d = (ATA_FIS_H2D*)CmdTbl->CommandFis;
        H2d->FisType = FIS_TYPE_H2D;
        H2d->CommandControl = 1;
        H2d->Command = ATA_READ_DMA_EX;
        H2d->Count = ReadBytes >> 9;
        H2d->Device = AHCI_DEVICE_LBA;
        H2d->Lba0 = Address;
        H2d->Lba1 = Address >> 16;
        H2d->Lba2 = Address >> 24;
        H2d->Lba3 = Address >> 40;

        CmdTbl->Prdt[0].DataBaseAddress = (UINT64)Buffer;
        CmdTbl->Prdt[0].DataByteCount = ReadBytes - 1;
        CmdTbl->Prdt[0].InterruptOnCompletion = 1;

        Port->PendingCommands[CommandSlot] = Thread;
        __SyncOr(&Port->Port->CommandIssue, CommandSlot);
    }
    for(;;) {
        // Waiting until read finishes
        RFTHREAD* Th = Port->PendingCommands;
        BOOL ThreadPending = FALSE;
        for(UINT i = 0;i<Port->Ahci->MaxCommandSlots;i++, Th++) {
            // Clear all acked reads
            if(*Th != Thread) continue;
            ThreadPending = TRUE;
            if((Port->DoneCommands & (1 << i))) {
                // Release Command Slot
                SystemDebugPrint(L"SATA_READ : Command Done");
                *Th = NULL;
                Port->DoneCommands &= ~(1 << i);
                Port->UsedCommandSlots &= ~(1 << i);
            }
        }
        if(!ThreadPending) break;
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

    UINT32 HbaReceivedIntStatus = GlobalInterruptStatus;
    for(int i = 0;i<Ahci->NumPorts;i++) {
        if(GlobalInterruptStatus & 1) {
            HBA_PORT* HbaPort = &Ahci->HbaPorts[i];
            AHCI_DEVICE_PORT* Port = &Ahci->Ports[i];
            SystemDebugPrint(L"Interrupt on PORT %d (%x)", (UINT64)i, HbaPort->PortSignature);
            if(HbaPort->InterruptStatus.D2HRegisterFisInterrupt) {
                
                SystemDebugPrint(L"D2H_REG_FIS_INT (TYPE : %x)", Ahci->Ports[i].ReceivedFis->D2h.FisType);
                if(!Port->FirstD2h) Port->FirstD2h = 1;

                DWORD CmdIssue = HbaPort->CommandIssue;
                RFTHREAD* Pending = Ahci->Ports[i].PendingCommands;
                for(UINT8 c = 0;c<Ahci->MaxCommandSlots;c++, Pending++) {
                    if(*Pending && *Pending != (RFTHREAD)-1 && !(CmdIssue & 1)) {
                        SystemDebugPrint(L"Command#%d Compeleted", c);
                        RFTHREAD Th = *Pending;
                        Ahci->Ports[i].DoneCommands |= (1 << c);
                        IoFinish(Th);
                    }
                    CmdIssue >>= 1;
                }
                HbaPort->InterruptStatus.D2HRegisterFisInterrupt = 1;
            }

            if(HbaPort->InterruptStatus.PioSetupFisInterrupt) SystemDebugPrint(L"PIO_SETUP");
            if(HbaPort->InterruptStatus.DmaSetupFisInterrupt) {
                SystemDebugPrint(L"DMA_SETUP_FIS");
                HbaPort->InterruptStatus.DmaSetupFisInterrupt = 1;
            }


            if(HbaPort->InterruptStatus.SetDeviceBitsInterrupt) {
                SystemDebugPrint(L"SET_DEVICE_BITS");
                HbaPort->InterruptStatus.SetDeviceBitsInterrupt = 1;
            }
            if(HbaPort->InterruptStatus.UnknownFisInterrupt) {
                SystemDebugPrint(L"UNKNOWN_FIS");
                HbaPort->SataError.UnknownFisType = 0;
            }
            if(HbaPort->InterruptStatus.DescriptorProcessed) {
                SystemDebugPrint(L"DESCRIPTOR_PROCESSED");
                HbaPort->InterruptStatus.DescriptorProcessed = 1;
            }
            if(HbaPort->InterruptStatus.PortConnectChangeStatus) {
                SystemDebugPrint(L"PORT_CONNECT_CHANGE");
                wr32(&Port->Port->SataError, *(UINT32*)&Port->Port->SataError);
            }
            if(HbaPort->InterruptStatus.DeviceMechanicalPresenceStatus) SystemDebugPrint(L"DEVICE_MECHANICAL_PRESENCE");
            if(HbaPort->InterruptStatus.PhyRdyChangeStatus) {
                SystemDebugPrint(L"PHY_RDI_CHANGE");
                // wr32(&HbaPort->SataError, -1);
                wr32(&Port->Port->SataError, *(UINT32*)&Port->Port->SataError);
            } 
            if(HbaPort->InterruptStatus.IncorrectPortMultiplierStatus) SystemDebugPrint(L"INCORRECT_PORT_MULTIPLIER");
            if(HbaPort->InterruptStatus.OverflowStatus) SystemDebugPrint(L"OVERFLOW");
            if(HbaPort->InterruptStatus.InterfaceNonFatalErrorStatus) {
                SystemDebugPrint(L"INTERFACE_NON_FATAL");
                HbaPort->InterruptStatus.InterfaceNonFatalErrorStatus = 1;
            }
            if(HbaPort->InterruptStatus.InterfaceFatalErrorStatus) {
                SystemDebugPrint(L"INTERFACE_FATAL");
                HbaPort->InterruptStatus.InterfaceFatalErrorStatus = 1;
            }
            if(HbaPort->InterruptStatus.HostBusDataErrorStatus) SystemDebugPrint(L"HOST_BUS_DATA_ERROR");
            if(HbaPort->InterruptStatus.HostBusFatalErrorStatus) SystemDebugPrint(L"HOST_BUS_FATAL_ERROR");
            if(HbaPort->InterruptStatus.TaskFileErrorStatus) {
                SystemDebugPrint(L"TASK_FILE_ERROR");
                HbaPort->InterruptStatus.TaskFileErrorStatus = 1;
            }
            if(HbaPort->InterruptStatus.ColdPortDetectStatus) SystemDebugPrint(L"COLD_PRESENCE_DETECT");
        }
        GlobalInterruptStatus >>= 1;
    }

    Ahci->Hba->InterruptStatus = HbaReceivedIntStatus;
    // SystemDebugPrint(L"INTH_COMPLETE");
}

DDKSTATUS DDKENTRY DriverEntry(RFDRIVER_OBJECT Driver){
    // SystemDebugPrint(L"SATA AHCI Driver Startup : %d Controllers Detected", Driver->NumDevices);
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


        RFAHCI_DEVICE Ahci = KeExtendedAlloc(KeGetCurrentThread(), ALIGN_VALUE(sizeof(AHCI_DEVICE), 0x1000), 0x1000, NULL, 0);
        if(!Ahci) return KERNEL_SERR_UNSUFFICIENT_MEMORY;
        KeMapMemory(Ahci, ALIGN_VALUE(sizeof(AHCI_DEVICE), 0x1000) >> 12, PM_MAP | PM_CACHE_DISABLE | PM_WRITE_THROUGH);
        ObjZeroMemory(Ahci);

        Ahci->Device = Device;
        Ahci->Hba = Hba;
        Ahci->HbaPorts = (HBA_PORT*)((char*)Hba + AHCI_PORTS_OFFSET);
        SetDeviceExtension(Device, Ahci);
        
         

        AhciReset(Ahci);

        if(Hba->ExtendedHostCapabilities.BiosOsHandoff
        ){
            SystemDebugPrint(L"AHCI : Bios Owned");
            Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore = 1;
            while(!Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore);
            while(Hba->BiosOsHandoffControlAndStatus.BiosBusy);
            while(Hba->BiosOsHandoffControlAndStatus.BiosOwnedSemaphore);
            SystemDebugPrint(L"AHCI Ownership Taken : BIOS_OWNED : %x, OS_OWNED : %x", (UINT64)Hba->BiosOsHandoffControlAndStatus.BiosOwnedSemaphore, (UINT64)Hba->BiosOsHandoffControlAndStatus.OsOwnedSemaphore);
        }

        if(Hba->HostCapabilities.x64AddressingCapability){
            SetDeviceFeature(Device, DEVICE_64BIT_ADDRESS_ALLOCATIONS);
            // SystemDebugPrint(L"AHCI Supports 64 Bit Memory Addresses.");
        }

        Ahci->MaxCommandSlots = Ahci->Hba->HostCapabilities.NumCommandSlots;
        

        



        SetInterruptService(Device, AhciInterruptHandler);
        
        Ahci->Hba->GlobalHostControl.InterruptEnable = 1;
    
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
            }
            PortsImplemented >>= 1;
        }
        
        for(UINT i = 0;i<Ahci->NumPorts;i++) {
            AHCI_DEVICE_PORT* Port = &Ahci->Ports[i];
            HBA_PORT* HbaPort = Port->Port;
            if(!Port->DeviceDetected) continue;
            SystemDebugPrint(L"SATA AHCI Device Detected (Port : %x). ATAPI = %x", i, HbaPort->CommandStatus.DeviceIsAtapi);
            if(HbaPort->CommandStatus.DeviceIsAtapi == 0 && HbaPort->SataStatus.DeviceDetection == 3) {
                char* Sector0 = malloc(0x1000);
                ATA_IDENTIFY_DEVICE_DATA IdentifyDevice = {0};
                ZeroMemory(Sector0, 0x1000);
                AHCI_COMMAND_ADDRESS CommandAddress = {0};
                CommandAddress.BaseAddress = Sector0;
                CommandAddress.NumBytes = 0x1000;//sizeof(ATA_IDENTIFY_DEVICE_DATA);
                KERNELSTATUS Status = AhciHostToDevice(Port, 0, ATA_READ_DMA, AHCI_DEVICE_LBA, 8, 1, &CommandAddress);
                AhciSataRead(Port, 40, 0x1000, Sector0);
                // SystemDebugPrint(L"MAX_LBA : %x , WWN : %s", IdentifyDevice.Max48BitLBA, IdentifyDevice.WorldWideName);
                SystemDebugPrint(L"STATUS = %x, SECTOR0 (%x) : %s", Status, *(UINT64*)Sector0, Sector0);
                SystemDebugPrint(L"SECTOR 1 (%x) : %s ||| SECTOR 2 (%x) : %s", *(UINT64*)(Sector0 + 0x200), Sector0 + 0x200, *(UINT64*)(Sector0 + 0x400), Sector0 + 0x400);
    
                // DRIVE_INFO* DriveInfo = malloc(sizeof(DRIVE_INFO));
                // ObjZeroMemory(DriveInfo);
            }
        }

        // SystemDebugPrint(L"AHCI Controller Intialized.");
    }
    while(1) Sleep(100000);
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

BOOL AhciComReset(AHCI_DEVICE_PORT* Port) {
    Port->FirstD2h = 0;
    wr32(&Port->Port->SataError, -1);
    Port->Port->SataControl.DeviceDetectionInitialization = 1;
    Sleep(3);
    Port->Port->SataControl.DeviceDetectionInitialization = 0;
    UINT Countdown = 25;
    while(!Port->FirstD2h) {
        Sleep(1);
        Countdown--;
        if(!Countdown) return FALSE;
    }
    return TRUE;
}


void AhciInitializePort(RFAHCI_DEVICE_PORT Port){
    UINT64 NumBytes = ALIGN_VALUE(sizeof(AHCI_COMMAND_LIST_ENTRY) * Port->Ahci->MaxCommandSlots + 0x100 + sizeof(AHCI_COMMAND_TABLE) * Port->Ahci->MaxCommandSlots + 0x100, 0x1000);
    Port->AllocatedBuffer = AllocateDeviceMemory(Port->Controller, NumBytes, 0x1000);
    KeMapMemory(Port->AllocatedBuffer, NumBytes >> 12, PM_MAP | PM_CACHE_DISABLE | PM_WRITE_THROUGH);
    AHCI_COMMAND_LIST_ENTRY* CommandList = (void*)Port->AllocatedBuffer;
    AHCI_COMMAND_TABLE* CommandTables = (void*)(ALIGN_VALUE((UINT64)Port->AllocatedBuffer + sizeof(AHCI_COMMAND_LIST_ENTRY) * Port->Ahci->MaxCommandSlots, 0x100)); // Set after command list (struct is 128 bytes aligned)
    AHCI_RECEIVED_FIS* ReceivedFis = (void*)(Port->AllocatedBuffer + NumBytes - 0x100);
    
    HBA_PORT* HbaPort = Port->Port;

    HbaPort->CommandStatus.Start = 0;
    HbaPort->CommandStatus.FisReceiveEnable = 0;
    while(HbaPort->CommandStatus.CurrentCommandSlot || HbaPort->CommandStatus.FisReceiveRunning);

    Port->FirstD2h = 0;


    Port->ReceivedFis = ReceivedFis;
    Port->CommandList = CommandList;
    Port->CommandTables = CommandTables;
    
    

  

    
    for(UINT i = 0;i<CMD_LIST_ENTRIES;i++){
        CommandList[i].PrdtLength = NUM_PRDT_PER_CMDTBL;
        CommandList[i].CommandTableAddress = (UINT64)&CommandTables[i];
    }
    HbaPort->FisBaseAddressLow = (UINT64)ReceivedFis;
    HbaPort->CommandListBaseAddressLow = (UINT64)CommandList;

    HbaPort->FisBaseAddressHigh = (UINT64)ReceivedFis >> 32;
    HbaPort->CommandListBaseAddressHigh = (UINT64)CommandList >> 32;

    // if(Port->Port->CommandStatus.ColdPresenceDetection) {
    //     Port->Port->CommandStatus.PowerOnDevice = 1;
    // }
    Port->Port->InterruptEnable.D2HRegisterFisInterruptEnable = 1;
    Port->Port->InterruptEnable.PioSetupFisInterruptEnable = 1;
    Port->Port->InterruptEnable.DmaSetupFisInterruptEnable = 1;
    Port->Port->InterruptEnable.SetDeviceBitsFisInterruptEnable = 1;
    Port->Port->InterruptEnable.UnknownFisInterruptEnable = 1;
    Port->Port->InterruptEnable.DescriptorProcessedInterruptEnable = 1;
    Port->Port->InterruptEnable.PortChangeInterruptEnable = 1;
    Port->Port->InterruptEnable.DeviceMechanicalPresenceEnable = 1;
    Port->Port->InterruptEnable.PhyRdyChangeInterruptEnable = 1;
    Port->Port->InterruptEnable.IncorrectPortMultiplierEnable = 1;
    Port->Port->InterruptEnable.OverflowEnable = 1;
    Port->Port->InterruptEnable.InterfaceNonFatalErrorEnable = 1;
    Port->Port->InterruptEnable.InterfaceFatalErrorEnable = 1;
    Port->Port->InterruptEnable.HostBusDataErrorEnable = 1;
    Port->Port->InterruptEnable.HostBusFatalErrorEnable = 1;
    Port->Port->InterruptEnable.TaskFileErrorEnable = 1;
    Port->Port->InterruptEnable.ColdPresenceDetectEnable = 1;

    wr32(&Port->Port->SataError, *(UINT32*)&Port->Port->SataError);

    Port->Port->CommandStatus.FisReceiveEnable = 1;
    Port->Port->CommandStatus.Start= 1;
    while(!Port->Port->CommandStatus.FisReceiveRunning);
    
    // *(DWORD*)&Port->Port->SataError = -1; // Clear SATA_ERROR

    
    BOOL DeviceDetected = 0;
    if(Port->Ahci->Hba->HostCapabilities.SupportsStaggeredSpinup) {
        Port->Port->CommandStatus.SpinupDevice = 1;
    }
        Port->Port->SataControl.DeviceDetectionInitialization = 1;
        Sleep(3);
        Port->Port->SataControl.DeviceDetectionInitialization = 0;

    // while(!Port->Port->InterruptStatus.PortConnectChangeStatus);
    // AhciComReset(Port);
    UINT Countdown = 20;
    for(;;) {
        // if(Port->FirstD2h && !HbaPort->SataStatus.DeviceDetection) break;
        if(!Countdown) break;
        Countdown--; 
        if(HbaPort->SataStatus.DeviceDetection == 3) {
            if(Port->Ahci->Hba->HostCapabilities.SupportsStaggeredSpinup) {
            }
            DeviceDetected = 1;
            break;
        }
        Sleep(1);
    }




    if(!DeviceDetected) {
        if(Port->Ahci->Hba->HostCapabilities.SupportsStaggeredSpinup) {
            Port->Port->CommandStatus.SpinupDevice = 0;
        }
        Port->DeviceDetected = 0;
        return;
    } else {
        Port->DeviceDetected = 1;
        // SystemDebugPrint(L"ATTACHED_DEVICE_ON_PORT");
    }
     // wait for first D2H FIS (Containing device signature)
    Countdown = 25;
    while(!Port->FirstD2h) {
        Countdown--;
        if(!Countdown) {
            if(Port->Port->PortSignature.SectorCount == 0x101) break; // Port signnature received but no interrupt delivered
            Port->DeviceDetected = 0;
            return;
        }
        Sleep(1);
    }



    *(DWORD*)&Port->Port->SataError = -1; // Clear SATA_ERROR

    if(*(DWORD*)&HbaPort->PortSignature == 0xEB140101) {
        HbaPort->CommandStatus.DeviceIsAtapi = 1;
    }


    // Wait for device initialization (transfer of initial FIS & Device Signature...)
    


    SystemDebugPrint(L"Port Signature : %x", HbaPort->PortSignature);


    // Enable Interrupts
    // Port->Port->CommandStatus.Start = 0; // Clear SATA_STATUS
    // BUFFERWRITE32(Port->Port->InterruptStatus, -1);

    

    Port->Port->CommandStatus.FisReceiveEnable = 1;
    Port->Port->CommandStatus.Start = 1;
    

}




int AhciIssueCommand(RFAHCI_DEVICE_PORT Port, UINT CommandIndex) {
    if(Port->PendingCommands[CommandIndex]) return -1; // Command slot is already in use
    
    Port->PendingCommands[CommandIndex] = KeGetCurrentThread();


    while(Port->Port->TaskFileData.Busy);
    SystemDebugPrint(L"AHCI : COMMAND_READY (PxIS : %x PxSERR : %x PxSCTL : %x PxSSTS : %x)", Port->Port->InterruptStatus, Port->Port->SataError, Port->Port->SataControl, Port->Port->SataStatus);
    __SyncOr(&Port->Port->CommandIssue, CommandIndex);
    while(!(Port->DoneCommands & (1 << CommandIndex))) IoWait();
    
    // Release Command Slot
    Port->DoneCommands &= ~(1 << CommandIndex);
    Port->PendingCommands[CommandIndex] = NULL;
    Port->UsedCommandSlots &= ~(1 << CommandIndex);
    SystemDebugPrint(L"AHCI_COMMAND_ACK_RECEIVED. (PxIS : %x PxSERR : %x PxSCTL : %x PxSSTS : %x)", Port->Port->InterruptStatus, Port->Port->SataError, Port->Port->SataControl, Port->Port->SataStatus);
    return 0;
}

UINT32 AhciAllocateCommand(RFAHCI_DEVICE_PORT Port){
    
    for(;;) {
    DWORD Mask = Port->Port->CommandIssue | Port->Port->SataActive;
        // Spin until a free command is found
        for(UINT i = 0;i<Port->Ahci->MaxCommandSlots;i++){
            if(!(Mask & 1) && !(Port->UsedCommandSlots & (1 << i))) {
                // It is a free command list
                if(!__SyncBitTestAndSet(&Port->UsedCommandSlots, i)) continue;
                return i;
            }
            Mask >>= 1;
        }
    }
}
KERNELSTATUS AhciHostToDevice(RFAHCI_DEVICE_PORT Port, UINT64 Lba, UINT8 Command, UINT8 Device, UINT16 Count, UINT16 NumCommandAddressDescriptors, AHCI_COMMAND_ADDRESS* CommandAddresses){    
    // if(!Count) return KERNEL_SERR_INVALID_PARAMETER;
    UINT32 CommandIndex = AhciAllocateCommand(Port);
    AHCI_COMMAND_LIST_ENTRY* CmdEntry = &Port->CommandList[CommandIndex];
    CmdEntry->CommandFisLength = sizeof(ATA_FIS_H2D) >> 2;
    CmdEntry->PrdtByteCount = Count << 9;

    AHCI_COMMAND_TABLE* CommandTable = &Port->CommandTables[CommandIndex];
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
        ObjZeroMemory(Prdt);
        Prdt->DataBaseAddress = (UINT64)CommandAddresses[i].BaseAddress;
        Prdt->DataByteCount = CommandAddresses[i].NumBytes - 1;
        Prdt->InterruptOnCompletion = 1;
        
    }
    
    return AhciIssueCommand(Port, CommandIndex);

}