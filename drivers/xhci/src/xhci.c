#include <ddk.h>
#include <kernelruntime.h>
#include <assemblydef.h>
#include <xhci.h>


DDKSTATUS DDKENTRY DriverEntry(RFDRIVER_OBJECT Driver) {
    // __cli();
    SystemDebugPrint(L"XHCI Driver Entry : Num Devices %d", Driver->NumDevices);
    while(1);
    for(UINT32 x = 0;x<Driver->NumDevices;x++) {
        RFDEVICE_OBJECT Device = Driver->Devices[x];
        BOOL Mmio = FALSE;
        void* XhciBaseAddress = PciGetBaseAddress(Device, 0, &Mmio);
        if(!Mmio) continue; // XHCI Must be memory mapped
        SystemDebugPrint(L"XHCI BASE ADDRESS : %x", XhciBaseAddress);
        XHCI_DEVICE* Xhci = malloc(sizeof(XHCI_DEVICE));
        ObjZeroMemory(Xhci);
        SetDeviceExtension(Device, Xhci);
        Xhci->CapabilityRegisters = XhciBaseAddress;
        KeMapMemory(Xhci->CapabilityRegisters, Xhci->CapabilityRegisters, 1, PM_MAP | PM_CACHE_DISABLE);
        Xhci->OperationalRegisters = (void*)((char*)XhciBaseAddress + Xhci->CapabilityRegisters->CapLength);
        KeMapMemory(Xhci->OperationalRegisters, Xhci->OperationalRegisters, 1, PM_MAP | PM_CACHE_DISABLE);

        SystemDebugPrint(L"XHCI Revision : %x", Xhci->CapabilityRegisters->HciVersion);
    }
    while(1);
}