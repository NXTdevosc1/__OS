#include <gdk.h>
#include <vmsvga/vmsga.h>
#include <kernelruntime.h>
#include <kintrinsic.h>
GDKSTATUS GDKAPI DriverEntry(RFDRIVER Driver){
    KERNELSTATUS Status = KERNEL_SOK;
    if(CheckPciExpress()){
        PCI_CONFIGURATION_HEADER* PciConfigurationHeader = NULL;
        // Check for supported GPU's
        // VMWARE SVGA
        
        if((PciConfigurationHeader = PciExpressFindDevice(PCI_VENDOR_ID_VMWARE, PCI_DEVICE_ID_VMWARE_SVGA2))){
            VmwareSvgaInitialize(PciConfigurationHeader);
        }
    }
    while(1);
    return 0;
}