#define EHCI_DRV__
#include <gdk.h>
#include <kernelruntime.h>
#include <usb.h>
// #include <ehci/ehci.h>
#include <ehci/usb2.h>
#include <assemblydef.h>

#define EHCI_ASSERT(Str) KeSetDeathScreen(0, Str, L"EHCI Driver Failure.", OS_SUPPORT_LNKW) // ...

#define SET_REGISTER32(Register, Value) (*(DWORD*)(&Register) = Value)

#define EHCI_INITIAL_MEMORY_SIZE(Ehci) (0x1000 /*1 Page for periodic*/ + EHCI_ASYNC_POOL_LENGTH)

void EhciSetupPorts(EHCI_DEVICE* Ehci);


UINT64 EhciEncodeBase(PCI_CONFIGURATION_HEADER* PciConfig){
    
    UINT32 BAR0 = PciConfig->BaseAddresses[0];
    UINT64 UsbBase = BAR0;
    if(BAR0 & PCI_BAR_IO) return 0;
    if(BAR0 & (PCI_BAR_64BIT)){ // Type field 10b (may be mapped into 64 bit address space)
        UsbBase = (BAR0 & ~(0xf) /*remove reserved bits*/) 
        | ((UINT64)PciConfig->BaseAddresses[1] << 32);
    }else{
        UsbBase &= ~(0xf); // remove reserved bits
    }
    return UsbBase;
}
char SimpleTextBuffer[120] = {0};

BOOL GetFrameListElementsCount(EHCI_DEVICE* Ehci){
    switch(Ehci->OperationnalRegisters->UsbCommand.FrameListSize){
        case 0:
        {
            Ehci->NumElementsPerFrameList = 1024;
            break;
        }
        case 1:
        {
            Ehci->NumElementsPerFrameList = 512;
            break;
        }
        case 2:
        {
            Ehci->NumElementsPerFrameList = 256;
            break;
        }
        default: return FALSE;
    }
    return TRUE;
}


/*
    Parameters : EHCI Device
    returns : Index in the periodic pointer list
*/
DWORD EhciAllocatePeriodicListPointer(EHCI_DEVICE* Ehci){
        for(;;){
            for(DWORD i = 0;i<Ehci->NumElementsPerFrameList;i++){
                if(Ehci->PeriodicPointerList[i].Terminate){
                    return i;
                }
            }
        }
    return (DWORD)-1;
}

void EhciHandleDeviceInterrupt(EHCI_DEVICE* Device){
    if(Device->OperationnalRegisters->UsbStatus.UsbInterrupt){
        SystemDebugPrint(L"EHCI Device Interrupt");
    }
    else if(Device->OperationnalRegisters->UsbStatus.UsbErrorInterrupt){
        SystemDebugPrint(L"EHCI Device Interrupt : ERROR");
    }
    else if(Device->OperationnalRegisters->UsbStatus.PortChangeDetect){
        SystemDebugPrint(L"EHCI Device Interrupt : PORT_CHANGE_DETECT (%d)", Device->OperationnalRegisters->UsbInterruptEnable.PortChangeInterruptEnable);
        EhciHandlePortChange(Device);
    }
    else if(Device->OperationnalRegisters->UsbStatus.HostSystemError){
        SystemDebugPrint(L"EHCI Device Interrupt : HOST_SYSTEM_ERROR");
    }
    EhciAckAllInterrupts(Device);
}

void __cdecl EhciInterruptHandler(_IN UINT64 IrqNumber, _IN RFINTERRUPT_STACK_FRAME InterruptStackFrame){
    SystemDebugPrint(L"EHCI Interrupt IRQ_NUM : %d INT_STACK : %x", IrqNumber, InterruptStackFrame);
    EHCI_DEVICE* Device = EhciDevices;
    for(UINT i = 0;i<NumEhciDevices;i++, Device++){
        if(Device->Set && Device->InterruptLine == IrqNumber){
            EhciHandleDeviceInterrupt(Device);            
        }
    }
}

BOOL EhciResetController(EHCI_DEVICE* Ehci){
    Ehci->OperationnalRegisters->UsbCommand.RunStop = 0; // Stop the USB Controller
    while(!Ehci->OperationnalRegisters->UsbStatus.HcHalted);
    EhciAckAllInterrupts(Ehci);
    Ehci->OperationnalRegisters->UsbCommand.HostControllerReset = 1;
    while(Ehci->OperationnalRegisters->UsbCommand.HostControllerReset);
    // All registers will be set to their initial value
    return TRUE;
}

#define EHCI_LINK_POINTER(Pointer) (((UINT32)(UINT64)Pointer) >> 5)



void* EhciAllocateMemory(EHCI_DEVICE* Ehci, UINT64 NumBytes, UINT32 Align){
    void* Heap = NULL;
    if(Ehci->QwordBitWidth){
        Heap = KeExtendedAlloc(NULL, NumBytes, Align, NULL, 0);
        if(!Heap) EHCI_ASSERT(L"Failed to allocate 64-BIT Physical Memory for EHCI USB(2.0) Device");
    }else{
        Heap = KeExtendedAlloc(NULL, NumBytes, Align, NULL, 0xFFFFFFFF - NumBytes - Align);
        if(!Heap) EHCI_ASSERT(L"Failed to allocate 32-BIT Physical Memory for EHCI USB(2.0) Device");
    }
    ZeroMemory(Heap, NumBytes);
    return Heap;
}

KERNELSTATUS EhciControllerInitialize(PCI_CONFIGURATION_HEADER* PciConfiguration, EHCI_DEVICE* Ehci){
    Ehci->EhciBase = EhciEncodeBase(PciConfiguration);
    if(!Ehci->EhciBase) return KERNEL_SERR;
    KeMapMemory((void*)Ehci->EhciBase, 1, PM_MAP | PM_CACHE_DISABLE);
    Ehci->CapabilityRegisters = (EHCI_CAPABLILITY_REGISTERS*)Ehci->EhciBase;
    Ehci->OperationnalRegisters = (EHCI_OPERATIONAL_REGISTERS*)(Ehci->EhciBase + Ehci->CapabilityRegisters->CapLength);
    
    itoa((UINT64)Ehci->EhciBase, SimpleTextBuffer, RADIX_HEXADECIMAL);

    Ehci->InterruptLine = PciConfiguration->InterruptLine;
    Ehci->InterruptPin = PciConfiguration->InterruptPin;
    

    // Check for Extended Capabilites (BIOS May have EHCI Ownership)
    if(Ehci->CapabilityRegisters->CapabilityParams.ExtendedCapabilitiesPtr){
        EHCI_LEGACY_SUPPORT_EXCAPABILITY* ExCapability = (void*)((UINT64)PciConfiguration + Ehci->CapabilityRegisters->CapabilityParams.ExtendedCapabilitiesPtr);
        for(;;){
            // SystemDebugPrint(L"EXTENDED_CAPABILITY : %x", ExCapability->CapabilityId);
            if(ExCapability->CapabilityId == 1){
                // EHCI_LEGACY_SUPPORT EXTENDED_CAPABILITY
                // SystemDebugPrint(L"LEGACY_SUPPORT_EXCAPABILITY");
                if(ExCapability->HcBiosOwnedSemaphore) SystemDebugPrint(L"HC OWNED BY BIOS");

                ExCapability->HcOsOwnedSemaphore = 1;
                while(!ExCapability->HcOsOwnedSemaphore);

                SystemDebugPrint(L"HC OWNERSHIP Taken By OS HC_OS_OWNED : %x, HC_BIOS_OWNED : %x", ExCapability->HcOsOwnedSemaphore, ExCapability->HcBiosOwnedSemaphore);
            }
            if(!ExCapability->NextEhciExCapabilityPtr) break;
            ExCapability = (void*)((UINT64)PciConfiguration + ExCapability->NextEhciExCapabilityPtr);
        }
    }else{
        SystemDebugPrint(L"EXTENDED_CAPABILITIES Not Supported");
    }


    // Reset the Host Controller

    Ehci->OperationnalRegisters->UsbCommand.RunStop = 0;
    Ehci->OperationnalRegisters->ConfigurationFlag = 0;

    if(!EhciResetController(Ehci)){
        SystemDebugPrint(L"Failed to reset EHCI Controller.");
        return KERNEL_SERR;
    }
    

    if(!GetFrameListElementsCount(Ehci)) return KERNEL_SERR;

    SystemDebugPrint(L"EHCI : PCI Command 0x%x", PciConfiguration->Command);
    PciConfiguration->Command = 0x17;
    if(PciConfiguration->Command & (1 << 10)){
        SystemDebugPrint(L"EHCI : PCI Interrupts are disabled in the PCI Configuration");
    }

    // if(Ehci->CapabilityRegisters->CapabilityParams.x64AddressingCapability){
    //     Ehci->QwordBitWidth = 1;
    //     Ehci->AssignedMemory = EhciAllocateMemory(Ehci, EHCI_INITIAL_MEMORY_SIZE(Ehci), 0x1000);
    //     Ehci->High32Bits = (DWORD)((UINT64)Ehci->AssignedMemory >> 32);
    //     Ehci->OperationnalRegisters->ControlDataSegment = Ehci->High32Bits;// Set High 32 Bits
    //     Ehci->BaseMemory = (UINT32)(UINT64)Ehci->AssignedMemory;
        
    //     // SystemDebugPrint(L"EHCI Is In 64 Bit Mode. Upper 64 Bit Address (CTRLDATASEG = %x)", Ehci->OperationnalRegisters->ControlDataSegment);
    // }else{
        Ehci->AssignedMemory = EhciAllocateMemory(Ehci, EHCI_INITIAL_MEMORY_SIZE(Ehci), 0x1000);
        Ehci->BaseMemory = (UINT32)(UINT64)Ehci->AssignedMemory;
        // SystemDebugPrint(L"EHCI Is In 32 Bit Mode.");
    // }
    
    // KeMapMemory(Ehci->AssignedMemory, EHCI_INITIAL_MEMORY_SIZE(Ehci) >> 12, PM_MAP);

    
    // SystemDebugPrint(L"Num Ports : %d", Ehci->CapabilityRegisters->StructuralParams.NumPorts);
    
    

    // Ehci->OperationnalRegisters->FrameListBaseAddress = (UINT32)(UINT64)Ehci->AssignedMemory;

    DWORD* FrameListEntry = Ehci->AssignedMemory;
    for(UINT i = 0;i<Ehci->NumElementsPerFrameList;i++, FrameListEntry++){
        *FrameListEntry = 1; // Set TERMINATE_BIT
    }


    Ehci->OperationnalRegisters->ControlDataSegment = 0;

    // Ehci->OperationnalRegisters->UsbCommand.InterruptThresholdControl = 1; // set to lowest value




    // // Setting up INTR
    Ehci->OperationnalRegisters->UsbInterruptEnable.UsbInterruptEnable = 1;
    Ehci->OperationnalRegisters->UsbInterruptEnable.HostSystemErrorEnable = 1;
    Ehci->OperationnalRegisters->UsbInterruptEnable.PortChangeInterruptEnable = 1;
    Ehci->OperationnalRegisters->UsbInterruptEnable.UsbErrorInterruptEnable = 1;
    Ehci->OperationnalRegisters->UsbInterruptEnable.InterruptOnAsyncAdvanceEnable = 1;
    

    Ehci->OperationnalRegisters->UsbCommand.InterruptOnAsyncAdvanceDoorbell = 0;
    Ehci->OperationnalRegisters->UsbCommand.AsyncScheduleParkModeEnable = 0;
    Ehci->OperationnalRegisters->UsbCommand.AsyncScheduleEnable = 0;
    Ehci->OperationnalRegisters->UsbCommand.PeriodicScheduleEnable = 0;


    Ehci->NumPorts = Ehci->CapabilityRegisters->StructuralParams.NumPorts;


    Ehci->OperationnalRegisters->ConfigurationFlag = 1;
    Ehci->OperationnalRegisters->UsbCommand.RunStop = 1; // Start the USB Controller

    // SystemDebugPrint(L"INT_LINE: %d, INT_PIN : %d", Ehci->InterruptLine, Ehci->InterruptPin);
    // EhciHandleDeviceInterrupt(Ehci);



    Ehci->Set = TRUE;
    // PciConfiguration->InterruptLine = Ehci->InterruptLine;
    Ehci->InterruptLine = 23;
    SystemDebugPrint(L"PCI_INTLINE : %x, INT_LINE : %x, INT_PIN : %x", PciConfiguration->InterruptLine, Ehci->InterruptLine, Ehci->InterruptPin);
    // for(UINT i = 0;i<24;i++){
    KERNELSTATUS Status = KeControlIrq(EhciInterruptHandler, Ehci->InterruptLine, IRQ_DELIVERY_NORMAL, 0);
    // }
    // if(Status != KERNEL_SOK && Status != KERNEL_SWR_IRQ_ALREADY_SET){
    //     EHCI_ASSERT(L"FAILED TO REGISTER ISR For EHCI (USB2) Controller");
    // }
    Ehci->Running = 1;
    // SystemDebugPrint(L"USBCMD : %x, USBSTS : %x, CONFIGFLAG : %x, INTR : %x, PORT0 : %x", Ehci->OperationnalRegisters->UsbCommand, Ehci->OperationnalRegisters->UsbStatus, Ehci->OperationnalRegisters->ConfigurationFlag, Ehci->OperationnalRegisters->UsbInterruptEnable, Ehci->OperationnalRegisters->PortStatusControl[0]);

    // if(!EhciCreateIsochronousTransferDescriptor(Ehci))
    //     EHCI_ASSERT(L"Failed to Create EHCI ITD.");
    
    
    SystemDebugPrint(L"EHCI Controller Successfully configurated. EHCI Object : 0x%x", Ehci);
    return KERNEL_SOK;
}



BOOL EhciResetPort(EHCI_DEVICE* Ehci, DWORD PortNumber){
    if(!Ehci || PortNumber >= Ehci->NumPorts) return FALSE;

    EHCI_PORT_STATUS_AND_CONTROL_REGISTER* Port = &Ehci->OperationnalRegisters->PortStatusControl[PortNumber];

    if(Port->PortOwner) return FALSE; // Cannot release port ownership

    // if(Ehci->CapabilityRegisters->StructuralParams.PortPowerControl
    // ){
    //     Port->PortPower = 1;
    //     SystemDebugPrint(L"PORT %d POWER ON.", PortNumber);
    // }



    // turn the controller off
    // Ehci->OperationnalRegisters->UsbCommand.RunStop = 0;
    // while(!Ehci->OperationnalRegisters->UsbStatus.HcHalted); // wait until hc is halted

    // Remove Suspended State From the port

    SystemDebugPrint(L"PERFORMING_PORT_RESET...");
    Sleep(100); // Wait device attachement

    Port->PortReset = 1;   


    Sleep(50); // Wait for 50 ms             
    Port->PortReset = 0;

    if(Port->PortReset) {
        SystemDebugPrint(L"PORT_RESET_DELAY EXCEEDED. Trying to reset the port...");
        while(Port->PortReset) Port->PortReset = 0;
        // return FALSE;
    }

    // Ehci->OperationnalRegisters->UsbCommand.RunStop = 1;

    SystemDebugPrint(L"Resetted High Speed USB2 (INTERFACE %d.%d) Port %d (480 mb/s)", Ehci->CapabilityRegisters->HciVersion >> 8, (UINT64)(UINT8)Ehci->CapabilityRegisters->HciVersion, PortNumber);

    Sleep(100); // Wait device attachement
    return TRUE;
}
BOOL EhciDisablePort(EHCI_DEVICE* Ehci, DWORD PortNumber){
    if(!Ehci || PortNumber >= Ehci->NumPorts) return FALSE;
    EHCI_PORT_STATUS_AND_CONTROL_REGISTER* Port = &Ehci->OperationnalRegisters->PortStatusControl[PortNumber];

    Port->PortEnabled = 0;
    return TRUE;
}



void EhciAckAllInterrupts(EHCI_DEVICE* Ehci){
    DWORD UsbStatus = *(DWORD*)&Ehci->OperationnalRegisters->UsbStatus;
    UsbStatus |= EHCI_STATUS_INTMASK; // set ints to 1 to ack them
    SET_REGISTER32(Ehci->OperationnalRegisters->UsbStatus, UsbStatus);
}