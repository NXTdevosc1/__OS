#include <ddk.h>
#include <kernelruntime.h>
#include <ahci.h>
#include <kintrinsic.h>
#include <assemblydef.h>
#include <ktime.h>


#define ATA_IDENTIFY_DEVICE_DMA 0xEE
#define ATA_IDENTIFY_DEVICE 0xEC

#define AHCI_READWRITE_MAX_NUMBYTES ((DWORD)0xFFFF << 16)

#define PORTxCMDxSTART 1
#define PORTxCMDxSUD (1 << 1)
#define PORTxCMDxPOD (1 << 2)
#define PORTxCMDxCLO (1 << 3)
#define PORTxCMDxFRE (1 << 4)
#define PORTxCMDxCCS (1 << 8)
#define PORTxCMDxMPSS (1 << 13)
#define PORTxCMDxFRR (1 << 14)
#define PORTxCMDxCR (1 << 15)
#define PORTxCMDxCPS (1 << 16)
#define PORTxCMDxPMA (1 << 17)
#define PORTxCMDxHPCP (1 << 18)
#define PORTxCMDxMPSAP (1 << 19)
#define PORTxCMDxCPD (1 << 20)
#define PORTxCMDxESP (1 << 21)
#define PORTxCMDxFISBSCP (1 << 22)
#define PORTxCMDxAPTSTE (1 << 23)
#define PORTxCMDxATAPI (1 << 24)
#define PORTxCMDxALPME (1 << 25)
#define PORTxCMDxASP (1 << 26)
#define PORTxCMDxICC 27


DDKSTATUS DDKENTRY DriverEntry(RFDRIVER_OBJECT Driver){
    // SystemDebugPrint(L"SATA AHCI Driver Startup : %d Controllers Detected", Driver->NumDevices);
    for(UINT ControllerIndex = 0;ControllerIndex<Driver->NumDevices;ControllerIndex++){
        RFDEVICE_OBJECT Device = Driver->Devices[ControllerIndex];
        SystemDebugPrint(L"Installing AHCI Controller %x ... ", Device);
        SetDeviceFeature(Device, DEVICE_FORCE_MEMORY_ALLOCATION);
        BOOL MMio = FALSE;
        // AllocatePciBaseAddress(Device, 5, AHCI_CONFIGURATION_PAGES, 0);
        
        HBA_REGISTERS* Hba = PciGetBaseAddress(Device, 5, &MMio);
        SystemDebugPrint(L"STAGE 2");

        KeMapMemory(Hba, Hba, AHCI_CONFIGURATION_PAGES, PM_MAP | PM_CACHE_DISABLE | PM_WRITE_THROUGH);
        if(!MMio) {
            WriteDeviceLogA(Device, "Device BAR is not Memory Mapped (BAR5.IO = 1)");
            SetDeviceStatus(Device, KERNEL_SERR_INCORRECT_DEVICE_CONFIGURATION);
            continue;
        }

        Hba->GlobalHostControl.AhciEnable = 1;

        // Check if BIOS is device owner and take device ownership


        RFAHCI_DEVICE Ahci = AllocatePoolEx(KeGetCurrentThread(), ALIGN_VALUE(sizeof(AHCI_DEVICE), 0x1000), 0x1000, NULL, 0);
        if(!Ahci) return KERNEL_SERR_UNSUFFICIENT_MEMORY;
        KeMapMemory(Ahci, Ahci, ALIGN_VALUE(sizeof(AHCI_DEVICE), 0x1000) >> 12, PM_MAP | PM_CACHE_DISABLE);
        ObjZeroMemory(Ahci);

        Ahci->Device = Device;
        Ahci->Hba = Hba;
        Ahci->HbaPorts = (HBA_PORT*)((char*)Hba + AHCI_PORTS_OFFSET);
        Ahci->Driver = Driver;
        SetDeviceExtension(Device, Ahci);
        


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

        AhciReset(Ahci);
        Ahci->MaxCommandSlots = Ahci->Hba->HostCapabilities.NumCommandSlots;
        

        


        SetInterruptService(Device, AhciInterruptHandler);
        
        Ahci->Hba->GlobalHostControl.InterruptEnable = 1;
    
        DWORD PortsImplemented = Ahci->Hba->PortsImplemented;
        UINT MaxPorts = Ahci->Hba->HostCapabilities.NumPorts + 1;
        for(UINT i = 0;i<MaxPorts;i++){
            if(PortsImplemented & 1){
                HBA_PORT* HbaPort = &Ahci->HbaPorts[i];
                KeMapMemory(HbaPort, HbaPort, 1, PM_MAP | PM_CACHE_DISABLE | PM_WRITE_THROUGH);
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
            SystemDebugPrint(L"SATA AHCI Device Detected (Port : %x). ATAPI = %x", i, HbaPort->CommandStatus & PORTxCMDxATAPI);
            if(Port->Atapi) {
                AhciInitAtapiDevice(Port);
            } else {
                AhciInitAtaDevice(Port);
            }
        }

        SystemDebugPrint(L"AHCI Controller Intialized.");
    }
    
    while(1) Sleep(100000);
}

void AhciReset(RFAHCI_DEVICE Ahci){
    // Turn off all ports
    {
        DWORD tmp = Ahci->Hba->PortsImplemented;
        for(int i = 0;i<0x20;i++) {
            if(tmp & 1) {
                Ahci->HbaPorts[i].CommandStatus &= ~PORTxCMDxSTART;
                while(Ahci->HbaPorts[i].CommandStatus & PORTxCMDxCR);
                Ahci->HbaPorts[i].CommandStatus &= ~PORTxCMDxFRE;
                while(Ahci->HbaPorts[i].CommandStatus & PORTxCMDxFRR);
            }
            tmp >>= 1;
        }
    }

    
    Ahci->Hba->GlobalHostControl.HbaReset = 1;
    while(Ahci->Hba->GlobalHostControl.HbaReset);
    Ahci->Hba->GlobalHostControl.AhciEnable = 1;


}

#define CMD_LIST_ENTRIES 0x20


void AhciInitializePort(RFAHCI_DEVICE_PORT Port){
    UINT64 NumBytes = ALIGN_VALUE(sizeof(AHCI_COMMAND_LIST_ENTRY) * Port->Ahci->MaxCommandSlots + 0x100 + sizeof(AHCI_COMMAND_TABLE) * Port->Ahci->MaxCommandSlots + 0x100, 0x1000);
    Port->AllocatedBuffer = AllocateDeviceMemory(Port->Controller, NumBytes, 0x1000);
    KeMapMemory(Port->AllocatedBuffer, Port->AllocatedBuffer, NumBytes >> 12, PM_MAP | PM_CACHE_DISABLE | PM_WRITE_THROUGH);
    AHCI_COMMAND_LIST_ENTRY* CommandList = (void*)Port->AllocatedBuffer;
    AHCI_COMMAND_TABLE* CommandTables = (void*)(ALIGN_VALUE((UINT64)Port->AllocatedBuffer + sizeof(AHCI_COMMAND_LIST_ENTRY) * Port->Ahci->MaxCommandSlots, 0x100)); // Set after command list (struct is 128 bytes aligned)
    AHCI_RECEIVED_FIS* ReceivedFis = (void*)(Port->AllocatedBuffer + NumBytes - 0x100);
    
    HBA_PORT* HbaPort = Port->Port;

    HbaPort->CommandStatus &= ~PORTxCMDxSTART;
    while(HbaPort->CommandStatus & PORTxCMDxCR);
    HbaPort->CommandStatus &= ~PORTxCMDxFRE;
    while(HbaPort->CommandStatus & PORTxCMDxFRR);
    Port->FirstD2h = 0;

    

    Port->ReceivedFis = ReceivedFis;
    Port->CommandList = CommandList;
    Port->CommandTables = CommandTables;
    
    

  

    
    for(UINT i = 0;i<CMD_LIST_ENTRIES;i++){
        CommandList[i].PrdtLength = 1;
        CommandList[i].CommandTableAddress = (UINT64)&CommandTables[i];
    }
    HbaPort->FisBaseAddressLow = (UINT64)ReceivedFis;
    HbaPort->CommandListBaseAddressLow = (UINT64)CommandList;

    HbaPort->FisBaseAddressHigh = (UINT64)ReceivedFis >> 32;
    HbaPort->CommandListBaseAddressHigh = (UINT64)CommandList >> 32;


    *(UINT32*)&Port->Port->SataError = -1;

    Port->Port->CommandStatus |= PORTxCMDxFRE;

    while(!(HbaPort->CommandStatus & PORTxCMDxFRR));
    
    Port->Port->InterruptEnable = -1;

    
    BOOL DeviceDetected = 0;


    if(Port->Ahci->Hba->HostCapabilities.SupportsStaggeredSpinup) {
        Port->Port->CommandStatus |= PORTxCMDxSUD;
        Sleep(100);
    }
    if(Port->Port->CommandStatus & PORTxCMDxCPD) {
        Port->Port->CommandStatus |= PORTxCMDxPOD;
    }
    if(Port->Ahci->Hba->HostCapabilities.SlumberStateCapable) {
        UINT32 v = Port->Port->CommandStatus;
        v &= ~((0x1F) << PORTxCMDxICC);
        v |= 1 << PORTxCMDxICC;
        Port->Port->CommandStatus = v;
    }
    BUFFERWRITE32(Port->Port->SataControl, 0x301);

    Sleep(5);

    BUFFERWRITE32(Port->Port->SataControl, 0x300);


    UINT Countdown = 20;
    for(;;) {
        if(!Countdown) break;
        Countdown--; 
        if(HbaPort->SataStatus.DeviceDetection == 3) {
            DeviceDetected = 1;
            break;
        }
        Sleep(1);
    }


    if(!DeviceDetected) {
        if(Port->Ahci->Hba->HostCapabilities.SupportsStaggeredSpinup) {
            Port->Port->CommandStatus &= ~PORTxCMDxSUD;
        }
        Port->DeviceDetected = 0;
        return;
    } else {
        Port->DeviceDetected = 1;
        // SystemDebugPrint(L"ATTACHED_DEVICE_ON_PORT");
    }
     // wait for first D2H FIS (Containing device signature)
    Countdown = 50;
    while(HbaPort->TaskFileData.Busy || HbaPort->TaskFileData.DataTransferRequested) {
        if(!Countdown) {
            DeviceDetected = 0;
            break;
        }
        Countdown--;
        Sleep(1);
    }
    Countdown = 25;
    while(!Port->FirstD2h) {
        Countdown--;
        if(!Countdown) {
            Port->DeviceDetected = 0;
            return;
        }
        Sleep(1);
    }



    *(DWORD*)&Port->Port->SataError = -1; // Clear SATA_ERROR

    
    if(Port->Port->PortSignature.Signature == 0xEB140101) {
        Port->Atapi = 1;
        Port->Port->CommandStatus |= PORTxCMDxATAPI;
        AHCI_COMMAND_LIST_ENTRY* e = Port->CommandList;
        for(UINT i = 0;i<32;i++, e++) {
            e->Atapi = 1;
        }
    }
    SystemDebugPrint(L"Port Signature : %x , ATAPI : %x", HbaPort->PortSignature, HbaPort->CommandStatus & PORTxCMDxATAPI);
    Port->Port->CommandStatus |= PORTxCMDxSTART | PORTxCMDxFRE;    
    while((Port->Port->CommandStatus & (PORTxCMDxCR | PORTxCMDxFRR)) != (PORTxCMDxCR | PORTxCMDxFRR)) Sleep(1);

}




