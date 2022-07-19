#include <gdk.h>
#include <kernelruntime.h>
#include <usb.h>

EHCI_DEVICE* EhciDevices = NULL;
UINT NumEhciDevices = 0;

GDKSTATUS GDKAPI DriverEntry(RFDRIVER Driver){
    NumEhciDevices = 10;
    EhciDevices = malloc(NumEhciDevices * sizeof(EHCI_DEVICE));
    ZeroMemory(EhciDevices, NumEhciDevices * sizeof(EHCI_DEVICE));
    UINT EHCIndex = 0;
    if(CheckPciExpress()){
        for(UINT Bus = 0;Bus < 8;Bus++){
            for(UINT Device = 0;Device < 32;Device++){
                for(UINT Function = 0;Function < 8;Function++){
                    RFPCI_CONFIGURATION_HEADER Header = PciExpressConfigurationRead(0, Bus, Device, Function);
                    if(Header->DeviceClass == PCI_USB_CLASS && Header->DeviceSubclass == PCI_USB_SUBCLASS){
                        switch(Header->ProgramInterface){
                            case UHCI_CONTROLLER:
                            {
                                SystemDebugPrint(L"Found UHCI Controller. Bus : %x, Device : %x, Function : %x", Bus, Device, Function);
                                break;
                            }
                            case OHCI_CONTROLLER:
                            {
                                SystemDebugPrint(L"Found OHCI Controller. Bus : %x, Device : %x, Function : %x", Bus, Device, Function);
                                break;
                            }
                            case EHCI_CONTROLLER:
                            {
                                SystemDebugPrint(L"Found EHCI Controller. Bus : %x, Device : %x, Function : %x", Bus, Device, Function);
                                EhciControllerInitialize(Header, &EhciDevices[EHCIndex]);
                                EHCIndex++;
                                break;
                            }
                            case XHCI_CONTROLLER:
                            {
                                SystemDebugPrint(L"Found XHCI Controller. Bus : %x, Device : %x, Function : %x", Bus, Device, Function);
                                break;
                            }
                        }
                    }
                }
            }
        }
        SystemDebugPrint(L"PCI Express Successfully enumerated.");
        // for(;;){
        //     for(UINT i = 0;i<NumEhciDevices;i++){
        //         if(EhciDevices[i].Set){
        //             EhciHandleDeviceInterrupt(&EhciDevices[i]);
        //         }
        //     }
        // }
    }
    while(1);

    return KERNEL_SOK;
}