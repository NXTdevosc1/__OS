// INTERRUPT ROUTINES CORE
#include <ehci/ehci.h>
#include <gdk.h>
#include <ehci/usb2.h>
#include <usb.h>
#include <kernelruntime.h>

KERNELSTATUS EhciHandleDeviceConnection(EHCI_DEVICE* Ehci, DWORD PortNumber){
    
    RFEHCI_ASYNC_QUEUE ControlQueue = EhciCreateAsynchronousQueue(Ehci, 0, 0, 0, USB_HIGH_SPEED);
    RFEHCI_ASYNC_TRANSFER ControlTransfer = EhciCreateControlTransfer(ControlQueue, USB_SETUP);
    if(!ControlTransfer || !ControlQueue) {
        SystemDebugPrint(L"Failed to create async control queue or async control transfer");
        return KERNEL_SERR;
    }
    USB_DEVICE_DESCRIPTOR DeviceDescriptor = {0};
    USB_DEVICE_REQUEST DeviceRequest = {0};
    DeviceRequest.Request = USB_DEVICE_GET_DESCRIPTOR;
    DeviceRequest.RequestType = 0x80;
    DeviceRequest.Value = 1 << 8;
    DeviceRequest.Length = 0x12;
    
    // SystemDebugPrint(L"USB_GET_DESCRIPTOR");

    int Status = EhciControlDeviceRequest(ControlTransfer, &DeviceRequest, 0x12, &DeviceDescriptor);
    
    if(Status != 0) return Status;
    
    
    // Set Device Address
    DeviceRequest.Request = USB_DEVICE_SET_ADDRESS;
    DeviceRequest.RequestType = 0;
    DeviceRequest.Value = EhciAllocateDeviceAddress(Ehci);
    UINT DeviceAddress = DeviceRequest.Value;
    DeviceRequest.Length = 0;

    // SystemDebugPrint(L"DEVICE_ADDRESS : %x", DeviceRequest.Value);
    Status = EhciControlDeviceRequest(ControlTransfer, &DeviceRequest, 0, NULL);
    if(Status != 0) return Status;
   
    // Set Device Address
    ControlQueue->QueueHead->DeviceAddress = DeviceAddress;

//    // Set Device configuration
//     DeviceRequest.Request = USB_DEVICE_SET_CONFIGURATION;
//     DeviceRequest.Value = 1;
//     Status = EhciControlDeviceRequest(ControlTransfer, &DeviceRequest, 0, NULL);

    // SystemDebugPrint(L"LENGTH : %x, DESCRIPTOR_TYPE : %x, EP0_MAX_PACKET_SIZE : %x", (UINT64)DeviceDescriptor.Length, (UINT64)DeviceDescriptor.DescriptorType, (UINT64)DeviceDescriptor.Endpoint0MaxPacketSize);
    SystemDebugPrint(L"PRODUCT_ID : %x, NUM_CONFIGS : %x", DeviceDescriptor.ProductId, DeviceDescriptor.NumConfigurations);
    SystemDebugPrint(L"CLASS : %x, SUB_CLASS : %x, VENDOR_ID : %x, DEVICE_PROTOCOL : %x", (UINT64)DeviceDescriptor.DeviceClass, (UINT64)DeviceDescriptor.DeviceSubclass, (UINT64)DeviceDescriptor.VendorId, (UINT64)DeviceDescriptor.DeviceProtocol);    
    USB_STRING_DESCRIPTOR String = {0};
    if(DeviceDescriptor.SerialNumber){
        EhciControlGetString(ControlTransfer, &String, DeviceDescriptor.SerialNumber);
        SystemDebugPrint(L"SERIAL_NUMBER : %ls", String.String);
    }
    if(DeviceDescriptor.Manufacturer){
        EhciControlGetString(ControlTransfer, &String, DeviceDescriptor.Manufacturer);
        SystemDebugPrint(L"MANUFACTURER : %ls", String.String);
    }

    if(DeviceDescriptor.Product){
        EhciControlGetString(ControlTransfer, &String, DeviceDescriptor.Product);
        SystemDebugPrint(L"PRODUCT_NAME : %ls", String.String);
    }
    
    return KERNEL_SOK;
}

void EhciHandlePortChange(EHCI_DEVICE* Ehci){
    for(UINT i = 0;i<Ehci->NumPorts;i++){
        
        if(Ehci->OperationnalRegisters->PortStatusControl[i].ConnectStatusChange){
            SystemDebugPrint(L"CONNECT STATUS CHANGE");
            if(Ehci->OperationnalRegisters->PortStatusControl[i].LineStatus == EHCI_KSTATE){
                Ehci->OperationnalRegisters->PortStatusControl[i].PortOwner = 0;
                if(Ehci->OperationnalRegisters->PortStatusControl[i].PortOwner) continue;
                SystemDebugPrint(L"PORT LINE IN KSTATE.");
            }
            if(!EhciResetPort(Ehci, i)){
                SystemDebugPrint(L"Failed to reset port %d", i);
            }
            if(!Ehci->OperationnalRegisters->PortStatusControl[i].PortEnabled) {
                SystemDebugPrint(L"Resetted port is not enabled.");
                continue;
            }


            if(Ehci->OperationnalRegisters->PortStatusControl[i].CurrentConnectStatus == 1){
                SystemDebugPrint(L"EHCI Device Connected to port %d. Line Status : %d", i, Ehci->OperationnalRegisters->PortStatusControl[i].LineStatus);
                EhciHandleDeviceConnection(Ehci, i);
            }else{
                SystemDebugPrint(L"EHCI Device Disconnected from port %d", i);
            }

            Ehci->OperationnalRegisters->PortStatusControl[i].ConnectStatusChange = 1;
        }

        if(Ehci->OperationnalRegisters->PortStatusControl[i].OverCurrentChange){
            SystemDebugPrint(L"EHCI Port : OVER_CURRENT_CHANGE");
            Ehci->OperationnalRegisters->PortStatusControl[i].OverCurrentChange = 1;
        }
        if(Ehci->OperationnalRegisters->PortStatusControl[i].PortEnableDisableChange){
            SystemDebugPrint(L"EHCI Port : ENABLE STATUS CHANGE (Enabled = %d)", Ehci->OperationnalRegisters->PortStatusControl[i].PortEnabled);
            Ehci->OperationnalRegisters->PortStatusControl[i].PortEnableDisableChange = 1;
        }
        if(Ehci->OperationnalRegisters->PortStatusControl[i].ForcePortResume){
            SystemDebugPrint(L"EHCI Port : FORCE_PORT_RESUME");
            Ehci->OperationnalRegisters->PortStatusControl[i].ForcePortResume = 0;
            if(Ehci->OperationnalRegisters->PortStatusControl[i].ForcePortResume) SystemDebugPrint(L"EHCI ENTER_LOOP");
            while(Ehci->OperationnalRegisters->PortStatusControl[i].ForcePortResume);
            SystemDebugPrint(L"EHCI Port : FORCE_PORT_RESUME HANDLING FINISHED");
        }
        // EhciResetPort(Ehci, i);
    }
}