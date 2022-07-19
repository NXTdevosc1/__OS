#pragma once
// PCI PROGRAM INTERFACE (USB2)
#define EHCI_CONTROLLER 0x20

#include <kerneltypes.h>
#include <pciexpressapi.h>

#pragma pack(push, 1)

typedef struct _EHCI_HCSPARAMS{
    DWORD NumPorts : 4;
    DWORD PortPowerControl : 1;
    DWORD Reserved0 : 2;
    DWORD PortRoutineRules : 1;
    DWORD NumPortsPerCompanionController : 4;
    DWORD NumCompanionControllers : 4;
    DWORD PortIndications : 1;
    DWORD Reserved1 : 3;
    DWORD DebugPortNumber : 4;
    DWORD Reserved2 : 8;
} EHCI_HCSPARAMS;

typedef struct _EHCI_HCCPARAMS{
    DWORD x64AddressingCapability : 1;
    DWORD ProgrammableFrameList : 1;
    DWORD AsyncScheduleParkCapability : 1;
    DWORD Reserved0 : 1;
    DWORD IsochronousSchedulingTreshold : 4;
    DWORD ExtendedCapabilitiesPtr : 8; // if 0 then not supported, else (must be over 0x40) offset in PCI Configuration SPACE to extended CAP.
    DWORD Reserved1 : 16;
} EHCI_HCCPARAMS;

typedef struct _EHCI_CAPABILITY_REGISTERS{
    char CapLength; // capablity register length (Add this to find beginning of the Operationnal Registers)
    char Reserved;
    WORD HciVersion; // Interface version number
    EHCI_HCSPARAMS StructuralParams; // Structural Parameters
    EHCI_HCCPARAMS CapabilityParams;
    QWORD CompanionPortRoute;
} EHCI_CAPABLILITY_REGISTERS;

typedef struct _EHCI_USB_COMMAND_REGISTER{
    DWORD RunStop : 1;
    DWORD HostControllerReset : 1;
    DWORD FrameListSize : 2; // 0 : 1024 elements, 01b : 512, 10b 256, 11b Reserved
    DWORD PeriodicScheduleEnable : 1;
    DWORD AsyncScheduleEnable : 1;
    DWORD InterruptOnAsyncAdvanceDoorbell : 1;
    DWORD LightHostControllerReset : 1;
    DWORD AsyncScheduleParkModeCount : 2;
    DWORD Reserved0 : 1;
    DWORD AsyncScheduleParkModeEnable : 1;
    DWORD Reserved1 : 4;
    DWORD InterruptThresholdControl : 8; // num microframes
    DWORD Reserved : 8;
} EHCI_USB_COMMAND_REGISTER;

typedef struct _EHCI_USB_STATUS_REGISTER{
    DWORD UsbInterrupt : 1;
    DWORD UsbErrorInterrupt : 1;
    DWORD PortChangeDetect : 1;
    DWORD FrameListRollOver : 1;
    DWORD HostSystemError : 1;
    DWORD InterruptOnAsyncAdvance : 1;
    DWORD Reserved0 : 6;
    DWORD HcHalted : 1;
    DWORD Reclamation : 1;
    DWORD PeriodicScheduleStatus : 1;
    DWORD AsyncScheduleStatus : 1;
    DWORD Reserved1 : 16;
} EHCI_USB_STATUS_REGISTER;

typedef struct _EHCI_USB_INTERRUPT_ENABLE_REGISTER{
    DWORD UsbInterruptEnable : 1;
    DWORD UsbErrorInterruptEnable : 1;
    DWORD PortChangeInterruptEnable : 1;
    DWORD FrameListRolloverEnable : 1;
    DWORD HostSystemErrorEnable : 1;
    DWORD InterruptOnAsyncAdvanceEnable : 1;
    DWORD Reserved : 26;
} EHCI_USB_INTERRUPT_ENABLE_REGISTER;

enum EHCI_PORT_INDICATOR_CONTROL{
    EHCI_PORT_INDICATOR_OFF = 0,
    EHCI_PORT_INDICATOR_AMBER,
    EHCI_PORT_INDICATOR_GREEN,
    EHCI_PORT_INDICATOR_UNDEFINED
};

typedef struct _EHCI_PORT_STATUS_AND_CONTROL_REGISTER{
    DWORD CurrentConnectStatus : 1; // set if device is present on port, otherwase no device is present
    DWORD ConnectStatusChange : 1;
    DWORD PortEnabled : 1;
    DWORD PortEnableDisableChange : 1;
    DWORD OverCurrentActive : 1;
    DWORD OverCurrentChange : 1;
    DWORD ForcePortResume : 1;
    DWORD Suspend : 1;
    DWORD PortReset : 1;
    DWORD Reserved0 : 1;
    DWORD LineStatus : 2;
    DWORD PortPower : 1;
    DWORD PortOwner : 1;
    DWORD PortIndicatorControl : 2; // if Port Indicator in HCS_PARAMS is 0 then writing to this bit has no effect (this changes the color of usb port indicators)
    DWORD PortTestControl : 4;
    DWORD WakeOnConnectEnable : 1;
    DWORD WakeOnOvercurrentEnable : 1;
    DWORD Reserved : 10;
} EHCI_PORT_STATUS_AND_CONTROL_REGISTER;

enum EHCI_FRAME_LIST_ELEMENT_POINTER_TYPE{
    EHCI_TYPE_ITD = 0,
    EHCI_TYPE_QUEUE_HEAD,
    EHCI_TYPE_SITD,
    EHCI_TYPE_FRAME_SPAN_TRAVERSAL_NODE
};

typedef struct _EHCI_OPERATIONAL_REGISTERS{
    EHCI_USB_COMMAND_REGISTER UsbCommand;
    EHCI_USB_STATUS_REGISTER UsbStatus;
    EHCI_USB_INTERRUPT_ENABLE_REGISTER UsbInterruptEnable;
    DWORD UsbFrameIndex;
    DWORD ControlDataSegment; // if 64 Bit addressing then use this as upper bits of USB Addresses
    DWORD FrameListBaseAddress;
    DWORD AsyncListAddress;
    char Reserved[36];
    DWORD ConfigurationFlag; // set when the configuration of the driver finishes
    EHCI_PORT_STATUS_AND_CONTROL_REGISTER PortStatusControl[]; // Num Ports
} EHCI_OPERATIONAL_REGISTERS;

typedef struct _EHCI_FRAME_LIST_LINK_POINTER{
    DWORD Terminate : 1; // T-BIT
    DWORD Type : 2;
    DWORD Reserved : 2;
    DWORD FrameListLinkPointer : 27;
} EHCI_FRAME_LIST_LINK_POINTER;

// For High Speed Devices

typedef struct _EHCI_ITD_TRANSACTION_STATUS_AND_CONTROL{
    DWORD TransactionOffset : 12;
    DWORD PageSelect : 3; // 0 to 6
    DWORD InterruptOnComplete : 1;
    DWORD TransactionLength : 12; // Maximum value is 0xC00 (3072)
    //DWORD Status : 4; The next bits specifies transaction status
    DWORD TransactionError : 1;
    DWORD BabbleDetected : 1;
    DWORD DataBufferError : 1;
    DWORD Active : 1; // of present bit (set by host software) (unset by controller on transaction finish)
} EHCI_ITD_TRANSACTION_STATUS_AND_CONTROL;

typedef struct _EHCI_ITD_BUFFER_POINTER_PAGE0 { // (Plus)
    DWORD DeviceAddress : 7;
    DWORD Reserved0 : 1;
    DWORD EndPointNumber : 4;
    DWORD BufferPointer : 20; // (Page 0) 4K Aligned Pointer to physical memory
} EHCI_ITD_BUFFER_POINTER_PAGE0;

typedef struct _EHCI_ITD_BUFFER_POINTER_PAGE1 { // (Plus)
    DWORD MaxPacketSize : 11; // Max is 0x400
    DWORD Direction : 1; // 0 = OUT, 1 = IN
    DWORD BufferPointer : 20;
} EHCI_ITD_BUFFER_POINTER_PAGE1;

typedef struct _EHCI_ITD_BUFFER_POINTER_PAGE2 { // (Plus)
    DWORD Mutli : 2; // Num transaction to be issued for this endpoint per micro-frame
    DWORD Reserved : 10;
    DWORD BufferPointer : 20;
} EHCI_ITD_BUFFER_POINTER_PAGE2;

typedef struct _EHCI_ITD_BUFFER_POINTER_PAGE3_6 { // (Plus)
    DWORD Reserved : 12;
    DWORD BufferPointer : 20;
} EHCI_ITD_BUFFER_POINTER_PAGE3_6;



typedef struct _EHCI_ISOCHRONOUS_TRANSFER_DESCRIPTOR{
    EHCI_FRAME_LIST_LINK_POINTER NextLinkPointer; // 32 Byte Aligned Pointer
    EHCI_ITD_TRANSACTION_STATUS_AND_CONTROL Transactions[8];
    DWORD DeviceAddress : 7;
    DWORD Reserved0 : 1;
    DWORD EndPointNumber : 4;
    DWORD BufferPointer0 : 20; // (Page 0)
    DWORD MaxPacketSize : 11;
    DWORD Direction : 1;
    DWORD BufferPointer1 : 20;
    DWORD Multi : 2;
    DWORD Reserved1 : 10;
    DWORD BufferPointer2 : 20;
    DWORD Reserved2 : 12;
    DWORD BufferPointer3 : 20;
    DWORD Reserved3 : 12;
    DWORD BufferPointer4 : 20;
    DWORD Reserved4 : 12;
    DWORD BufferPointer5 : 20;
    DWORD Reserved5 : 12;
    DWORD BufferPointer6 : 20;
    // Set for 64 BIT Mode HC (Bits 63:32 For BufferPointerPages)
    DWORD ExtendedBufferPointer0;
    DWORD ExtendedBufferPointer1;
    DWORD ExtendedBufferPointer2;
    DWORD ExtendedBufferPointer3;
    DWORD ExtendedBufferPointer4;
    DWORD ExtendedBufferPointer5;
    DWORD ExtendedBufferPointer6;
} EHCI_ISOCHRONOUS_TRANSFER_DESCRIPTOR;


/*
KEYWORDS:
iTD: Isochronus Transfer Descriptor
siTD : Split Transaction Isochronous Transfer Descriptor
qTD : Queue Element Transfer Descriptor

*/
// Split Transaction Isochronous Transfer Descriptor
typedef struct _EHCI_SITD{
    EHCI_FRAME_LIST_LINK_POINTER NextLinkPointer;
    // siTD Endpoint Capabilities/Characteristics
    struct {
        DWORD DeviceAddress : 7;
        DWORD Reserved0 : 1;
        DWORD EndpointNumber : 4;
        DWORD Reserved1 : 4;
        DWORD HubAddress : 7;
        DWORD Reserved2 : 1;
        DWORD PortNumber : 7;
        DWORD Direction : 1; // 0 = OUT, 1 = IN
    } Characteristics;

    struct {
        DWORD SplitStartMask : 8;
        DWORD SplitCompletionMask : 8;
        DWORD Reserved : 16;
    } MicroframeScheduleControl;

    struct {
        // Status
        DWORD Reserved0 : 1;
        DWORD SplitTransactionState : 1; // 0 = do start split, 1 = do complete split
        DWORD MissedMicroFrame : 1;
        DWORD TransactionError : 1;
        DWORD BabbleDetected : 1;
        DWORD DataBufferError : 1;
        DWORD Error : 1;
        DWORD Active : 1;
        // Control
        DWORD FrameCompleteSplitProgressMask : 8;
        DWORD TotalBytesToTransfer : 10;
        DWORD Reserved1 : 4;
        DWORD PageSelect : 1;
        DWORD InterruptOnComplete : 1;
    } TransferStatusControl;

    DWORD CurrentOffset : 12;
    DWORD BufferPointer0 : 20;

    DWORD TransactionCount : 3; // Max is 6
    DWORD TransactionPosition : 2; // 0 = all, 1 = begin, 2 = mid, 3 = end
    DWORD Reserved0 : 7;
    DWORD BufferPointer1 : 20;

    EHCI_FRAME_LIST_LINK_POINTER BackLinkPointer; // bits 1:4 are reserved
    // Set for 64 BIT HC
    DWORD ExtendedBufferPointer0;
    DWORD ExtendedBufferPointer1;
} EHCI_SITD;

typedef struct _EHCI_QTD{
    DWORD NextQtdPointer; // bits 1 - 4 are reserved
    DWORD AlternateNextQtdPointer; // used by host controller

    // struct {
        // Status
        DWORD PingState : 1; // 0 = do out, 1 = do ping
        DWORD SplitTransactionState : 1;
        DWORD MissedMicroframes : 1;
        DWORD TransactionError : 1;
        DWORD BabbleDetected : 1;
        DWORD DataBufferError : 1;
        DWORD Halted : 1;
        DWORD Active : 1;
        // Token
        DWORD PidCode : 2; //0 = OUT Token, 1 = IN TOKEN, 2 = SETUP TOKEN, 3 = RESERVED
        DWORD ErrorCounter : 2;
        DWORD CurrentPage : 3;
        DWORD InterruptOnComplete : 1;
        DWORD TotalBytesToTransfer : 15;
        DWORD DataToggle : 1;
    // } QtdToken;

    DWORD BufferPointer0; // no alignment required

    DWORD Reserved0 : 12;
    DWORD BufferPointer1 : 20;
    DWORD Reserved1 : 12;
    DWORD BufferPointer2 : 20;
    DWORD Reserved2 : 12;
    DWORD BufferPointer3 : 20;
    DWORD Reserved3 : 12;
    DWORD BufferPointer4 : 20;

    // For 64 BIT Mode HC
    DWORD ExtendedBufferPointer0;
    DWORD ExtendedBufferPointer1;
    DWORD ExtendedBufferPointer2;
    DWORD ExtendedBufferPointer3;
    DWORD ExtendedBufferPointer4;
} EHCI_QTD;

enum EHCI_ENDPOINT_SPEED{
    EHCI_ENDPOINT_FULL_SPEED = 0, // 12 Mb/s
    EHCI_ENDPOINT_LOW_SPEED = 1, // 1.5Mb/s
    EHCI_ENDPOINT_HIGH_SPEED = 2, // 480 Mb/s
    EHCI_ENDPOINT_RESERVED = 3
};

enum EHCI_DIRECTION{
    EHCI_OUT = 0,
    EHCI_IN,
    EHCI_SETUP
};

enum EHCI_LINE_STATUS{
    EHCI_SE0 = 0,
    EHCI_JSTATE = 2,
    EHCI_KSTATE = 1, // Release ownership of the port, low speed device
    EHCI_LINE_STATUS_UNDEFINED = 3
};

// the main data structure for asynchronous transfers (uses a round robin algorithm with HorizontalLinkPointer)
typedef struct _EHCI_QUEUE_HEAD{
    DWORD HorizontalLinkPointer;
    // struct {
        DWORD DeviceAddress : 7;
        DWORD InactivateOnNextTransaction : 1;
        DWORD EndpointNumber : 4;
        DWORD EndpointSpeed : 2; // 0 = FullSpeed, 1 = low speed, 2 = HighSpeed, 3 = reserved
        DWORD DataToggleControl : 1;
        DWORD HeadOfReclamationListFlag : 1;
        DWORD MaxPacketLength : 11; // max is 0x400 (1024) 1K
        DWORD ControlEndpointFlag : 1;
        DWORD NakCountReload : 4;
    // } EndpointCharacteristcs;

    // struct {
        DWORD InterruptScheduleMask : 8;
        DWORD SplitCompletionMask : 8;
        DWORD HubAddress : 7;
        DWORD PortNumber : 7;
        DWORD HighBandwidthPipeMultiplier : 2; // num transactions to be issued for this endpoint per micro-frame
    // } EndpointCapabilities;
    DWORD CurrentQtdPointer; // Used only by Host Controller
    
    // Overlay area for EHC
    struct {
        DWORD NextQtdPointer;
        DWORD AlternateNextQtdPointer; // bits 4:1 Nake Counter

        // Endpoint Characteristics & Capabilities
            // struct {
        // Status
        DWORD PingState : 1; // 0 = do out, 1 = do ping
        DWORD SplitTransactionState : 1;
        DWORD MissedMicroframes : 1;
        DWORD TransactionError : 1;
        DWORD BabbleDetected : 1;
        DWORD DataBufferError : 1;
        DWORD Halted : 1;
        DWORD Active : 1;
        // Token
        DWORD PidCode : 2; //0 = OUT Token, 1 = IN TOKEN, 2 = SETUP TOKEN, 3 = RESERVED
        DWORD ErrorCounter : 2;
        DWORD CurrentPage : 3;
        DWORD InterruptOnComplete : 1;
        DWORD TotalBytesToTransfer : 15;
        DWORD DataToggle : 1;
    // } QtdToken;

        DWORD BufferPointer0; // Not aligned
            
        DWORD SplitTransactionCompleteSplitProgress : 8;
        DWORD Reserved0 : 4;
        DWORD BufferPointer1 : 20;
        
        DWORD SplitTransactionFrameTag : 5;
        DWORD SentBytes : 7;
        DWORD BufferPointer2 : 20;
        
        DWORD Reserved2 : 12;
        DWORD BufferPointer3 : 20;
        DWORD Reserved3 : 12;
        DWORD BufferPointer4 : 20;

        // For 64 BIT Mode HC
        DWORD ExtendedBufferPointer0;
        DWORD ExtendedBufferPointer1;
        DWORD ExtendedBufferPointer2;
        DWORD ExtendedBufferPointer3;
        DWORD ExtendedBufferPointer4;
    } HcOverlayArea;
} EHCI_QUEUE_HEAD;

typedef struct _EHCI_LEGACY_SUPPORT_EXCAPABILITY{
    UINT32 CapabilityId : 8; // Set to 1
    UINT32 NextEhciExCapabilityPtr : 8;
    UINT32 HcBiosOwnedSemaphore : 1;
    UINT32 Reserved0 : 7;
    UINT32 HcOsOwnedSemaphore : 1;
    UINT32 Reserved1 : 7;
} EHCI_LEGACY_SUPPORT_EXCAPABILITY;

#pragma pack(pop)

// EHCI MUTEX BITS

#ifdef EHCI_DRV__
#define ENTER_MUTEX(Ehci, BitOff) __SpinLockSyncBitTestAndSet(&Ehci->Mutex, BitOff)
#define EXIT_MUTEX(Ehci, BitOff) __BitRelease(&Ehci->Mutex, BitOff)
#define EHCI_MUTEX_ALLOCQH 0
#define EHCI_MUTEX_ALLOCQTD 1
#define EHCI_MUTEX_MANAGEASYNCQUEUE 2
#define EHCI_MUTEX_MANAGEDEVICE 3
#define EHCI_MUTEX_MANAGEASYNCTRANSFERS 4
#endif

typedef struct _EHCI_DEVICE EHCI_DEVICE;
typedef struct _EHCI_ASYNC_QUEUE EHCI_ASYNC_QUEUE, *RFEHCI_ASYNC_QUEUE;
typedef struct _EHCI_ASYNC_TRANSFER EHCI_ASYNC_TRANSFER, *RFEHCI_ASYNC_TRANSFER;

typedef struct _EHCI_ASYNC_QUEUE{
    BOOL Present;
    UINT Id;
    EHCI_DEVICE* Ehci;
    EHCI_QUEUE_HEAD* QueueHead;
    RFEHCI_ASYNC_TRANSFER FirstTransfer;
    RFEHCI_ASYNC_TRANSFER LastTransfer;
} EHCI_ASYNC_QUEUE, *RFEHCI_ASYNC_QUEUE;

typedef enum _EHCI_ASYNC_TRANSFER_FLAGS{
    EHCI_AT_FIRSTCONTROL_DONE = 1,
    EHCI_AT_DATA1 = 2
} EHCI_ASYNC_TRANSFER_FLAGS;

typedef struct _EHCI_ASYNC_TRANSFER{
    BOOL Present;
    UINT Id;
    UINT64 Flags;
    RFEHCI_ASYNC_QUEUE AsyncQueue;
    EHCI_QTD* Qtd;
    void* Buffer;
    RFEHCI_ASYNC_TRANSFER NextTransfer;
} EHCI_ASYNC_TRANSFER, *RFEHCI_ASYNC_TRANSFER;

#define EHCI_MAX_QH 0x100
#define EHCI_MAX_QTD 0x200
#define EHCI_MAX_PORTS 16

typedef struct _EHCI_DEVICE{
    BOOL                        Set;
    BOOL                        QwordBitWidth; // if set then EHC is in 64 Bit Mode
    PCI_CONFIGURATION_HEADER*   PciConfiguration;
    BOOL                        Running;
    UINT64                      EhciBase;
    EHCI_CAPABLILITY_REGISTERS* CapabilityRegisters;
    EHCI_OPERATIONAL_REGISTERS* OperationnalRegisters;
    void*                       AssignedMemory;
    UINT32                      BaseMemory; // if 32 bit mode then this = Assigned Memory, Otherwise this is the lower 32 bits of Assigned Memory, the upper 32 bits are set in CTRLDSEG register
    DWORD                       NumElementsPerFrameList;
    DWORD                       NumPorts;
    unsigned char               InterruptLine;
    unsigned char               InterruptPin;
    EHCI_FRAME_LIST_LINK_POINTER* PeriodicPointerList;
    DWORD                       High32Bits;
    RFEHCI_ASYNC_QUEUE          LastAsyncQueue;
    UINT64                      Mutex;
    EHCI_ASYNC_QUEUE            AsyncQueues[EHCI_MAX_QH];
    EHCI_ASYNC_TRANSFER         AsyncTransfers[EHCI_MAX_QTD];
    UINT64 DeviceAddressMap[2]; 
} EHCI_DEVICE;


// ALLOCATED POOL for QH & QTD
#define EHCI_ALIGNED_QH_SIZE 0x60
#define EHCI_ALIGNED_QTD_SIZE 0x40


#define EHCI_PERIODIC_START 0
#define EHCI_QH_START 0x1000
#define EHCI_QTD_START 0x7000

#define EHCI_ASYNC_POOL_LENGTH (EHCI_MAX_QH * EHCI_ALIGNED_QH_SIZE + EHCI_MAX_QTD * EHCI_ALIGNED_QTD_SIZE)

#define EHCI_STATUS_USB_INT 1
#define EHCI_STATUS_USB_ERRINT 2
#define EHCI_STATUS_PORT_CHANGE_DETECT 4
#define EHCI_STATUS_HOST_SYSTEM_ERROR 0x10
#define EHCI_STATUS_INTMASK 0x17

KERNELSTATUS EhciControllerInitialize(PCI_CONFIGURATION_HEADER* PciConfiguration, EHCI_DEVICE* Ehci);

BOOL EhciResetPort(EHCI_DEVICE* Ehci, DWORD PortNumber);
BOOL EhciDisablePort(EHCI_DEVICE* Ehci, DWORD PortNumber);

// internal
void* EhciAllocateMemory(EHCI_DEVICE* Ehci, UINT64 NumBytes, UINT32 Align);

void EhciHandlePortChange(EHCI_DEVICE* Ehci);
BOOL EhciInitializeQueueHead(EHCI_DEVICE* Ehci, EHCI_QUEUE_HEAD* QueueHead);
EHCI_QTD* EhciAllocateQTD(EHCI_DEVICE* Ehci);
EHCI_QUEUE_HEAD* EhciAllocateQueueHead(EHCI_DEVICE* Ehci);


// Endpoint Number 0 Sets the control flag on the queue head
RFEHCI_ASYNC_QUEUE EhciCreateAsynchronousQueue(EHCI_DEVICE* Ehci, UINT DeviceAddress, UINT PortNumber, UINT EndpointNumber, UINT EndpointSpeed /*IN, OUT, SETUP*/);

RFEHCI_ASYNC_TRANSFER EhciCreateControlTransfer(RFEHCI_ASYNC_QUEUE AsyncQueue, UINT Direction);
RFEHCI_ASYNC_TRANSFER EhciCreateInterruptTransfer(RFEHCI_ASYNC_QUEUE AsyncQueue);
RFEHCI_ASYNC_TRANSFER EhciCreateBulkTransfer(RFEHCI_ASYNC_QUEUE AsyncQueue);


int EhciWriteAsyncTransfer(RFEHCI_ASYNC_TRANSFER Transfer, UINT64 NumBytes, UINT Pid, void* Buffer);
int EhciReadAsyncTransfer(RFEHCI_ASYNC_TRANSFER Transfer, UINT64* NumBytes, void* Buffer, UINT64 Timeout /*set to 0 if no timeout*/);


BOOL EhciRemoveAsynchronousQueue(RFEHCI_ASYNC_QUEUE AsyncQueue);

void EhciAckAllInterrupts(EHCI_DEVICE* Ehci);

BOOL EhciSetAsyncTransferDirection(RFEHCI_ASYNC_TRANSFER Transfer, DWORD Direction);
BOOL EhciSetAsyncQueuePort(RFEHCI_ASYNC_QUEUE AsyncQueue, DWORD PortNumber);
BOOL EhciSetDataToggle(RFEHCI_ASYNC_QUEUE AsyncQueue, BOOL DataToggle);

int EhciControlDeviceRequest(RFEHCI_ASYNC_TRANSFER Transfer, void* DeviceRequest, UINT NumBytes, void* Buffer);

int EhciControlGetString(RFEHCI_ASYNC_TRANSFER ControlTransfer, void* StringDescriptor, UINT StringIndex);

unsigned char EhciAllocateDeviceAddress(EHCI_DEVICE* Ehci);
BOOL EhciCloseAsyncQueue(RFEHCI_ASYNC_QUEUE AsyncQueue);

void EhciHandleDeviceInterrupt(EHCI_DEVICE* Device);