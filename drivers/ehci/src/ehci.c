#include <ddk.h>
#include <usb.h>
#include <kernelruntime.h>
#include <ehci.h>

#define EHCI_INITIAL_MEMORY_SIZE(Ehci) (0x1000 /*1 Page for periodic*/ + EHCI_ASYNC_POOL_LENGTH)

void EhciInterruptHandler(RFDRIVER_OBJECT Driver, RFINTERRUPT_INFORMATION InterruptInformation) {
    SystemDebugPrint(L"EHCI Interrupt ! (Device : %x)", InterruptInformation->Device);
}

DDKSTATUS DDKENTRY DriverEntry(RFDRIVER_OBJECT DriverObject){
    while(1) Sleep(1000);
    SystemDebugPrint(L"EHCI Driver Startup : EHCI Device Detected. Num Devices : %d", DriverObject->NumDevices);  

    for(UINT i = 0;i<DriverObject->NumDevices;i++){
        BOOL isMMIO = FALSE;
        RFDEVICE_OBJECT Device = DriverObject->Devices[i];
        void* Bar0 = PciGetBaseAddress(Device, 0, &isMMIO);
        SystemDebugPrint(L"Device %x, EHC Base Address : %x", Device, Bar0);

        // It's assumed that BAR0 of the device is MMIO
        if(!isMMIO) {
            WriteDeviceLogA(Device, "Device BAR is not Memory Mapped (BAR0.IO = 1)");
            SetDeviceStatus(Device, KERNEL_SERR_INCORRECT_DEVICE_CONFIGURATION);
            continue;
        }

        EHCI_DEVICE* Ehc = malloc(sizeof(EHCI_DEVICE));
        if(!Ehc) {
            SetDeviceStatus(Device, KERNEL_SERR_UNSUFFICIENT_MEMORY);
            continue;
        }

        ObjZeroMemory(Ehc);
        SetDeviceExtension(Device, Ehc);

        KeMapMemory(Bar0, 1, PM_MAP | PM_CACHE_DISABLE);
        Ehc->EhciBase = Bar0;
        Ehc->CapabilityRegisters = Ehc->EhciBase;
        Ehc->OperationnalRegisters = (EHCI_OPERATIONAL_REGISTERS*)((char*)Ehc->EhciBase + Ehc->CapabilityRegisters->CapLength);
        
        // Take ownership from bios

        if(Ehc->CapabilityRegisters->CapabilityParams.ExtendedCapabilitiesPtr){
            UINT32 _excap = PciDeviceConfigurationRead32(Device, Ehc->CapabilityRegisters->CapabilityParams.ExtendedCapabilitiesPtr);
            EHCI_LEGACY_SUPPORT_EXCAPABILITY* ExtendedCapabilty = (EHCI_LEGACY_SUPPORT_EXCAPABILITY*)&_excap;
            for(;;){
                if(ExtendedCapabilty->CapabilityId == 1 && ExtendedCapabilty->HcBiosOwnedSemaphore){
                    SystemDebugPrint(L"EHCI: BIOS Owned, taking Ownership");
                    ExtendedCapabilty->HcOsOwnedSemaphore = 1;
                    while(!ExtendedCapabilty->HcOsOwnedSemaphore);
                    SystemDebugPrint(L"HC OWNERSHIP Taken By OS HC_OS_OWNED : %x, HC_BIOS_OWNED : %x", ExtendedCapabilty->HcOsOwnedSemaphore, ExtendedCapabilty->HcBiosOwnedSemaphore);
                }
                if(!ExtendedCapabilty->NextEhciExCapabilityPtr) break;

                _excap = PciDeviceConfigurationRead32(Device, ExtendedCapabilty->NextEhciExCapabilityPtr);
            }
        }

        
        // Reset the Enhanced Host Controller
        ResetEhc(Ehc);
        
        // Setup Device Interrupts
        // PciEnableInterrupts(Device);
        SetInterruptService(Device, EhciInterruptHandler);
        // Ehc->IrqNumber = PciGetInterruptNumber(Device);
        // if(KERNEL_ERROR(KeControlIrq(EhcInterruptHandler, Ehc->IrqNumber, IRQ_DELIVERY_NORMAL, 0))) {
        //     WriteDeviceLogA(Device, "Device IRQ Control Failed. IRQ May be controlled by another process");
        //     SetDeviceStatus(Device, KERNEL_SERR);
        //     continue;
        // }

        if(Ehc->CapabilityRegisters->CapabilityParams.x64AddressingCapability){
            Ehc->QwordBitWidth = 1;
            SetDeviceFeature(Device, DEVICE_64BIT_ADDRESS_ALLOCATIONS);
        }

        Ehc->AssignedMemory = AllocateDeviceMemory(Device, EHCI_INITIAL_MEMORY_SIZE(Ehc), 0x1000);
        if(!Ehc->AssignedMemory) {
            SetDeviceStatus(Device, KERNEL_SERR_UNSUFFICIENT_MEMORY);
            continue;
        }

        Ehc->High32Bits  = (UINT64)Ehc->AssignedMemory >> 32;
        Ehc->OperationnalRegisters->ControlDataSegment = Ehc->High32Bits;

        // Set EHC Interrupt Enable Register
        UINT Intr = BUFFERREAD32(Ehc->OperationnalRegisters->UsbInterruptEnable);
        Intr |= 0x1F;
        BUFFERWRITE32(Ehc->OperationnalRegisters->UsbInterruptEnable, Intr);
        

        Ehc->OperationnalRegisters->UsbCommand.AsyncScheduleEnable = 0;
        Ehc->OperationnalRegisters->UsbCommand.PeriodicScheduleEnable = 0;

        Ehc->OperationnalRegisters->ConfigurationFlag = 1;
        Ehc->OperationnalRegisters->UsbCommand.RunStop = 1;
        SystemDebugPrint(L"EHC %x (EHCI_DEVICE_EXTENSION : %x) Successfully initialized", DriverObject->Devices[i], Ehc);
    }
    while(1);
}

void __cdecl EhcInterruptHandler(UINT64 InterruptNumber, RFINTERRUPT_STACK_FRAME InterruptStack){
    SystemDebugPrint(L"EHC Interrupt");   
}

void ResetEhc(EHCI_DEVICE* Ehc){
    Ehc->OperationnalRegisters->UsbCommand.RunStop = 0; // Stop the USB Controller
    while(!Ehc->OperationnalRegisters->UsbStatus.HcHalted);
    EhciAckAllInterrupts(Ehc);
    Ehc->OperationnalRegisters->UsbCommand.HostControllerReset = 1;
    while(Ehc->OperationnalRegisters->UsbCommand.HostControllerReset);
    // All registers will be set to their initial value
}
void EhciAckAllInterrupts(EHCI_DEVICE* Ehci){
    DWORD UsbStatus = BUFFERREAD32(Ehci->OperationnalRegisters->UsbStatus);
    UsbStatus |= EHCI_STATUS_INTMASK; // set ints to 1 to ack them
    BUFFERWRITE32(Ehci->OperationnalRegisters->UsbStatus, UsbStatus);
}
