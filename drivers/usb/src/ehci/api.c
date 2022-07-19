
#define EHCI_DRV__
#include <ehci/ehci.h>
#include <ehci/usb2.h>
#include <gdk.h>
#include <usb.h>
#include <kernelruntime.h>

void EhcRegisterWrite(DWORD* Register, DWORD Value){
    *Register = Value;
}

RFEHCI_ASYNC_QUEUE EhciOpenAsyncQueue(EHCI_DEVICE* Ehci){
    ENTER_MUTEX(Ehci, EHCI_MUTEX_MANAGEASYNCQUEUE);
    for(UINT i = 0;i<EHCI_MAX_QH;i++){
        if(!Ehci->AsyncQueues[i].Present){
            Ehci->AsyncQueues[i].Present = TRUE;
            Ehci->AsyncQueues[i].Id = i;
            Ehci->AsyncQueues[i].Ehci = Ehci;

            EXIT_MUTEX(Ehci, EHCI_MUTEX_MANAGEASYNCQUEUE);
            Ehci->AsyncQueues[i].QueueHead = (EHCI_QUEUE_HEAD*)((UINT64)Ehci->AssignedMemory + EHCI_QH_START + i * EHCI_ALIGNED_QH_SIZE);
            EHCI_QUEUE_HEAD* qh = Ehci->AsyncQueues[i].QueueHead;
            // do this before queue linking
            qh->HcOverlayArea.NextQtdPointer = 1;
            qh->HcOverlayArea.AlternateNextQtdPointer = 1;
            qh->HorizontalLinkPointer = 1 | (EHCI_TYPE_QUEUE_HEAD << 1);;
            if(Ehci->LastAsyncQueue){
                Ehci->LastAsyncQueue->QueueHead->HorizontalLinkPointer = ((UINT32)(UINT64)Ehci->AsyncQueues[i].QueueHead) | (EHCI_TYPE_QUEUE_HEAD << 1);
            }else{
                Ehci->AsyncQueues[i].QueueHead->HeadOfReclamationListFlag = 1;
                EhcRegisterWrite((DWORD*)&Ehci->OperationnalRegisters->AsyncListAddress, (DWORD)(UINT64)Ehci->AsyncQueues[i].QueueHead);
                SystemDebugPrint(L"ASYNC_LIST_ADDR : %x", Ehci->OperationnalRegisters->AsyncListAddress);
                Ehci->OperationnalRegisters->UsbCommand.AsyncScheduleEnable = 1;
            }
            Ehci->LastAsyncQueue = &Ehci->AsyncQueues[i];
            
            return &Ehci->AsyncQueues[i];
        }
    }
    EXIT_MUTEX(Ehci, EHCI_MUTEX_MANAGEASYNCQUEUE);
    return NULL;
}

RFEHCI_ASYNC_TRANSFER OpenAsyncTransfer(EHCI_DEVICE* Ehci, RFEHCI_ASYNC_QUEUE AsyncQueue){
    ENTER_MUTEX(Ehci, EHCI_MUTEX_MANAGEASYNCTRANSFERS);
    for(UINT i = 0;i<EHCI_MAX_QTD;i++){
        if(!Ehci->AsyncTransfers[i].Present){
            Ehci->AsyncTransfers[i].Present = TRUE;
            Ehci->AsyncTransfers[i].Id = i;
            Ehci->AsyncTransfers[i].AsyncQueue = AsyncQueue;
            Ehci->AsyncTransfers[i].Qtd = (EHCI_QTD*)((UINT64)Ehci->AssignedMemory + EHCI_QTD_START + i * EHCI_ALIGNED_QTD_SIZE);
            EHCI_QTD* qtd = Ehci->AsyncTransfers[i].Qtd;
            qtd->NextQtdPointer = 1;
            qtd->AlternateNextQtdPointer = 1;

            if(!AsyncQueue->FirstTransfer){

                AsyncQueue->FirstTransfer = &Ehci->AsyncTransfers[i];
                AsyncQueue->LastTransfer = AsyncQueue->FirstTransfer;
                qtd->NextQtdPointer = ((UINT32)(UINT64)AsyncQueue->FirstTransfer->Qtd);

                AsyncQueue->QueueHead->HcOverlayArea.NextQtdPointer = (UINT32)(UINT64)AsyncQueue->FirstTransfer->Qtd;
            }else{
                AsyncQueue->LastTransfer->NextTransfer = &Ehci->AsyncTransfers[i];
                Ehci->AsyncTransfers[i].Qtd->NextQtdPointer = ((UINT32)(UINT64)AsyncQueue->FirstTransfer->Qtd);
                AsyncQueue->LastTransfer->Qtd->NextQtdPointer = ((UINT32)(UINT64)Ehci->AsyncTransfers[i].Qtd);
                AsyncQueue->LastTransfer = &Ehci->AsyncTransfers[i];
            }



            EXIT_MUTEX(Ehci, EHCI_MUTEX_MANAGEASYNCTRANSFERS);
            return &Ehci->AsyncTransfers[i];
        }
    }
    EXIT_MUTEX(Ehci, EHCI_MUTEX_MANAGEASYNCTRANSFERS);
    return NULL;
}

USBSTATUS EhciRunQueue(RFEHCI_ASYNC_QUEUE Queue){
    if(!Queue->FirstTransfer) return USB_STATUS_INVALID_PARAMETER;
    Queue->QueueHead->HcOverlayArea.NextQtdPointer = (UINT32)(UINT64)Queue->FirstTransfer->Qtd;
    RFEHCI_ASYNC_TRANSFER Transfer = Queue->FirstTransfer;
    for(;;){
        if(!Transfer) break;
        
        while(Transfer->Qtd->Active);

        SystemDebugPrint(L"EHCI_RUN_QUEUE : Transfer 0x%x : DONE", Transfer);
        Transfer = Transfer->NextTransfer;

    }
    return USB_STATUS_SUCCESS;
}

RFEHCI_ASYNC_QUEUE EhciCreateAsynchronousQueue(EHCI_DEVICE* Ehci, UINT DeviceAddress, UINT PortNumber, UINT EndpointNumber, UINT EndpointSpeed /*IN, OUT, SETUP*/){
    if(EndpointSpeed > USB_HIGH_SPEED) return NULL;
    RFEHCI_ASYNC_QUEUE AsyncQueue = EhciOpenAsyncQueue(Ehci);
    if(!AsyncQueue) return NULL;
    

    AsyncQueue->QueueHead->EndpointNumber = EndpointNumber;
    AsyncQueue->QueueHead->DeviceAddress = DeviceAddress;
    AsyncQueue->QueueHead->MaxPacketLength = 0x400;
    AsyncQueue->QueueHead->HighBandwidthPipeMultiplier = 1;
    AsyncQueue->QueueHead->DataToggleControl = 1;
    AsyncQueue->QueueHead->PortNumber = PortNumber;
    AsyncQueue->QueueHead->NakCountReload = 1;
    

    if(EndpointSpeed == USB_LOW_SPEED){
        AsyncQueue->QueueHead->EndpointSpeed = EHCI_ENDPOINT_LOW_SPEED;
    }else if(EndpointSpeed == USB_FULL_SPEED){
        AsyncQueue->QueueHead->EndpointSpeed = EHCI_ENDPOINT_FULL_SPEED;
    }else if(EndpointSpeed == USB_HIGH_SPEED){
        AsyncQueue->QueueHead->EndpointSpeed = EHCI_ENDPOINT_HIGH_SPEED;
    }

    if(EndpointNumber == 0){
        // AsyncQueue->QueueHead->ControlEndpointFlag = 1;
        AsyncQueue->QueueHead->MaxPacketLength = 0x40;
    }

    return AsyncQueue;
}

BOOL EhciCloseAsyncQueue(RFEHCI_ASYNC_QUEUE AsyncQueue){
    // Get Previous Queue
    EHCI_DEVICE* Ehc = AsyncQueue->Ehci;
    if(!AsyncQueue->Id){
        for(UINT i = 1;i<EHCI_MAX_QH;i++){
            if(Ehc->AsyncQueues[i].Present){
                // Disable Async Schedule
                Ehc->OperationnalRegisters->UsbCommand.AsyncScheduleEnable = 0;
                while(Ehc->OperationnalRegisters->UsbStatus.AsyncScheduleStatus);

                // Set Async Base
                Ehc->AsyncQueues[i].QueueHead->HeadOfReclamationListFlag = 1;
                Ehc->OperationnalRegisters->AsyncListAddress = (UINT32)(UINT64)Ehc->AsyncQueues[i].QueueHead;

                ObjZeroMemory(AsyncQueue->QueueHead);
                ObjZeroMemory(AsyncQueue);

                Ehc->LastAsyncQueue = &Ehc->AsyncQueues[i];

                // Re-enable Async Schedule
                Ehc->OperationnalRegisters->UsbCommand.AsyncScheduleEnable = 1;
                return TRUE;
            }
        }
        // if there is no Async Queue
        Ehc->OperationnalRegisters->UsbCommand.AsyncScheduleEnable = 0;
        while(Ehc->OperationnalRegisters->UsbStatus.AsyncScheduleStatus);
        Ehc->OperationnalRegisters->AsyncListAddress = 0;

        ObjZeroMemory(AsyncQueue->QueueHead);
        ObjZeroMemory(AsyncQueue);

        Ehc->LastAsyncQueue = NULL;
        return TRUE;
    }
    RFEHCI_ASYNC_QUEUE PreviousQueue = NULL;
    RFEHCI_ASYNC_QUEUE NextQueue = NULL;
    for(UINT i = AsyncQueue->Id - 1;i>=0;i--){
        if(Ehc->AsyncQueues[i].Present){
            PreviousQueue = &Ehc->AsyncQueues[i];
            break;
        }
    }
    for(UINT i = AsyncQueue->Id + 1;i<EHCI_MAX_QH;i++){
        if(Ehc->AsyncQueues[i].Present){
            NextQueue = &Ehc->AsyncQueues[i];
            break;
        }
    }

    if(PreviousQueue){
        if(NextQueue){
            PreviousQueue->QueueHead->HorizontalLinkPointer = ((UINT32)(UINT64)NextQueue->QueueHead) | (EHCI_TYPE_QUEUE_HEAD << 1);
        }else{
            PreviousQueue->QueueHead->HorizontalLinkPointer = 1 | (EHCI_TYPE_QUEUE_HEAD << 1);;
        }
    }else{
        if(NextQueue){
            // Set Next QUEUE As the first queue head
            Ehc->OperationnalRegisters->UsbCommand.AsyncScheduleEnable = 0;
            while(Ehc->OperationnalRegisters->UsbStatus.AsyncScheduleStatus);
            NextQueue->QueueHead->HeadOfReclamationListFlag = 1;
            Ehc->OperationnalRegisters->AsyncListAddress = (UINT32)(UINT64)NextQueue->QueueHead;

            Ehc->OperationnalRegisters->UsbCommand.AsyncScheduleEnable = 1;
        }else{
            // Disable Async Schedule
            Ehc->OperationnalRegisters->UsbCommand.AsyncScheduleEnable = 0;
            while(Ehc->OperationnalRegisters->UsbStatus.AsyncScheduleStatus);
            Ehc->OperationnalRegisters->AsyncListAddress = 0;
        }
    }
    ObjZeroMemory(AsyncQueue->QueueHead);
    ObjZeroMemory(AsyncQueue);


    return TRUE;
}

RFEHCI_ASYNC_TRANSFER EhciCreateControlTransfer(RFEHCI_ASYNC_QUEUE AsyncQueue, UINT Direction){
    if(Direction > USB_DIRECTION_MAX) return NULL;
    RFEHCI_ASYNC_TRANSFER Transfer = OpenAsyncTransfer(AsyncQueue->Ehci, AsyncQueue);
    if(!Transfer) return NULL;
    Transfer->Qtd->PidCode = Direction;

    char* Buffer = EhciAllocateMemory(AsyncQueue->Ehci, AsyncQueue->QueueHead->MaxPacketLength, 0);
    Transfer->Buffer = (void*)Buffer;


    Transfer->Qtd->BufferPointer0 = (UINT32)(UINT64)Buffer;
    Transfer->Qtd->ExtendedBufferPointer0 = (UINT32)((UINT64)Buffer >> 32);

    return Transfer;
}


int EhciWriteAsyncTransfer(RFEHCI_ASYNC_TRANSFER Transfer, UINT64 NumBytes, UINT Pid, void* Buffer){
    
    return 0;
}

int EhciReadAsyncTransfer(RFEHCI_ASYNC_TRANSFER Transfer, UINT64* NumBytes, void* Buffer, UINT64 Timeout /*set to 0 if no timeout*/){
    return 0;
}


BOOL EhciSetAsyncTransferDirection(RFEHCI_ASYNC_TRANSFER Transfer, DWORD Direction){
    if(Direction > USB_DIRECTION_MAX) return FALSE;
    if(Direction == USB_OUT){
        Transfer->Qtd->PidCode = 0;
    }else if(Direction == USB_IN){
        Transfer->Qtd->PidCode = 1;
    }else if(Direction == USB_SETUP){
        Transfer->Qtd->PidCode = 2;
    }
    return TRUE;
}
BOOL EhciSetAsyncQueuePort(RFEHCI_ASYNC_QUEUE AsyncQueue, DWORD PortNumber){
    AsyncQueue->QueueHead->PortNumber = PortNumber;
    return TRUE;
}

BOOL EhciSetDataToggle(RFEHCI_ASYNC_QUEUE AsyncQueue, BOOL DataToggle){
    AsyncQueue->QueueHead->DataToggleControl = DataToggle;
    return TRUE;
}


#define CURRENT_QTD_PTR(qtd) (void*)((UINT64)qtd->BufferPointer0 | ((UINT64)qtd->ExtendedBufferPointer0 << 32))

int EhciControlWrite(RFEHCI_ASYNC_TRANSFER Transfer, void* Buffer, UINT NumBytes){
    memcpy(CURRENT_QTD_PTR(Transfer->Qtd), Buffer, NumBytes);
    Transfer->Qtd->TotalBytesToTransfer = NumBytes;
    Transfer->Qtd->Active = 1;
    for(UINT64 i = 0;Transfer->Qtd->Active;i++){
        if(Transfer->Qtd->TransactionError){
            SystemDebugPrint(L"TRANSACTION_ERROR");
            return 0;
        }
        if(Transfer->Qtd->DataBufferError){
            SystemDebugPrint(L"DATA_BUFFER_ERROR");
            return 0;
        }
        if(Transfer->Qtd->BabbleDetected){
            SystemDebugPrint(L"BABBLE_DETECTED");
            return 0;
        }
        if(Transfer->Qtd->Halted){
            SystemDebugPrint(L"QTD_HALTED.");
        }
        if(i > 1000) {
            SystemDebugPrint(L"EHCI CONTROL WRITE : Device Response time exceeded. (ACTIVE = %x)", Transfer->Qtd->Active);
            for(UINT c = 0;c<10;c++){
                SystemDebugPrint(L"ASYNCQ : %x, QH_CQTD : %x, QH_NQTD : %x, QTD : %x, ASYNCADDR : %x", Transfer->AsyncQueue->QueueHead, Transfer->AsyncQueue->QueueHead->CurrentQtdPointer, Transfer->AsyncQueue->QueueHead->HcOverlayArea.NextQtdPointer, Transfer->Qtd, Transfer->AsyncQueue->Ehci->OperationnalRegisters->AsyncListAddress);
                Sleep(100);
            }
            return 0;
        }
        Sleep(1);
    }
    return 1;
}

int EhciControlRead(RFEHCI_ASYNC_TRANSFER Transfer, void* Buffer, UINT8 NumBytes){
    // Transfer->Qtd->BufferPointer0 = (UINT32)(UINT64)Transfer->Buffer;
    // Transfer->Qtd->ExtendedBufferPointer0 = (UINT32)((UINT64)Transfer->Buffer >> 32);
    void* Src = CURRENT_QTD_PTR(Transfer->Qtd);
    Transfer->Qtd->TotalBytesToTransfer = NumBytes;
    Transfer->Qtd->DataToggle = 1;
    Transfer->Qtd->Active = 1;
    for(UINT64 i = 0;Transfer->Qtd->Active;i++){
        if(Transfer->Qtd->TransactionError){
            SystemDebugPrint(L"TRANSACTION_ERROR");
            return 0;
        }
        if(Transfer->Qtd->DataBufferError){
            SystemDebugPrint(L"DATA_BUFFER_ERROR");
            return 0;
        }
        if(Transfer->Qtd->BabbleDetected){
            SystemDebugPrint(L"BABBLE_DETECTED");
            return 0;
        }
        if(Transfer->Qtd->Halted){
            SystemDebugPrint(L"QTD_HALTED.");
        }

        if(i > 1000) {
            SystemDebugPrint(L"EHCI CONTROL WRITE : Device Response time exceeded. (ACTIVE = %x)", Transfer->Qtd->Active);
            return 0;
        }
        Sleep(1);
    }
    memcpy(Buffer, Src, NumBytes); 
    
    Transfer->Qtd->DataToggle = 0;

    return 1;
}

int EhciControlDeviceRequest(RFEHCI_ASYNC_TRANSFER Transfer, void* DeviceRequest, UINT NumBytes, void* Buffer){
    


    // Refresh some data

    Transfer->Qtd->BufferPointer0 = (UINT32)(UINT64)Transfer->Buffer;
    Transfer->Qtd->ExtendedBufferPointer0 = (UINT32)((UINT64)Transfer->Buffer >> 32);
    Transfer->Qtd->DataToggle = 0;
    Transfer->Qtd->CurrentPage = 0;

    Transfer->AsyncQueue->QueueHead->PortNumber = 0;


    Transfer->Qtd->PidCode = EHCI_SETUP;
    // Send DEVICE_REQUEST

    if(EhciControlWrite(Transfer, DeviceRequest, 8) != 1) return -1;

    // Set PID_CODE to IN & Receive data from the device

    Transfer->Qtd->PidCode = EHCI_IN;
    if(EhciControlRead(Transfer, Buffer, NumBytes) != 1) return -1;


    return 0;
}

int EhciControlGetString(RFEHCI_ASYNC_TRANSFER ControlTransfer, void* StringDescriptor, UINT StringIndex){
    USB_DEVICE_REQUEST Request = {0};
    Request.Request = USB_DEVICE_GET_DESCRIPTOR;
    Request.RequestType = 0x80;
    Request.Value = 3 << 8 | StringIndex;
    Request.Length = 8; // STEP 1 : GetStringLength
    int Status = EhciControlDeviceRequest(ControlTransfer, &Request, 8, StringDescriptor);
    if(Status != 0) return Status;
    USB_STRING_DESCRIPTOR* StringDesc = StringDescriptor;
    Request.Length = StringDesc->Length; // STEP 2 : GetString By StringDescriptorLength
    ZeroMemory(StringDesc, Request.Length + 2);
    Status = EhciControlDeviceRequest(ControlTransfer, &Request, Request.Length, StringDesc);
    return Status;
}

unsigned char EhciAllocateDeviceAddress(EHCI_DEVICE* Ehci){
    unsigned char DeviceAddress = 1; // Addresses must start with 1
    UINT64 ShiftValue = 0;
    if(Ehci->DeviceAddressMap[0] != (UINT64)-1){
        ShiftValue = Ehci->DeviceAddressMap[0];
        for(UINT i = 0;i<64;i++, DeviceAddress++){
            if(!(ShiftValue & 1)){
                Ehci->DeviceAddressMap[0] |= 1 << (DeviceAddress - 1);
                return DeviceAddress;
            }
            ShiftValue >>= 1;
        }
        return 0;
    }else if(Ehci->DeviceAddressMap[1] != (UINT64)0x7FFFFFFFFFFFFFFF /*Higher bit is ignored*/){
        ShiftValue = Ehci->DeviceAddressMap[1];
        for(UINT i = 0;i<63;i++, DeviceAddress++){
            if(!(ShiftValue & 1)){
                Ehci->DeviceAddressMap[1] |= 1 << (DeviceAddress - 1);
                return DeviceAddress + 64;
            }
            ShiftValue >>= 1;
        }
        return 0;
    }

    return 0;
}