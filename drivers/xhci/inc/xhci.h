#pragma once
#include <kerneltypes.h>


typedef struct _TRANSFER_REQUEST_BLOCK {
    UINT64 DataBufferPointer;
    UINT32 Status;
    UINT32 Control;
} TRANSFER_REQUEST_BLOCK;

typedef struct _XHCI_CAPABILITY_REGISTERS {
    UINT8 CapLength;
    UINT8 Reserved;
    UINT16 HciVersion;
    // Structural Parameters
    struct {
        UINT32 NumDeviceSlots : 8;
        UINT32 NumInterrupters : 11;
        UINT32 Reserved : 5;
        UINT32 NumPorts : 8;
    } HcsParams1;
    struct {
        UINT32 IsochronousSchedulingThreshold : 4;
        UINT32 EventRingSegmentTableMax : 4;
        UINT32 Reserved : 13;
        UINT32 MaxScratchpadBuffersHigh : 5;
        UINT32 ScratchpadRestore : 1;
        UINT32 MaxScratchpadBuffersLow : 5;
    } HcsParams2;
    struct {
        UINT32 U1DeviceExitLatency : 8;
        UINT32 Reserved : 8;
        UINT32 U2DeviceExitLatency : 16;
    } HcsParams3;
    // Capability Parameter 1
    struct {
        UINT32 x64Addressing : 1;
        UINT32 BwNegotiation : 1;
        UINT32 ContextSize : 1; // if set then eXHC uses 64 BYTE Context Data structures otherwise it uses only 32 byte
        UINT32 PortPowerControl : 1;
        UINT32 PortIndicators : 1;
        UINT32 LightHcResetCapability : 1;
        UINT32 LatencyToleranceMessagingCapability : 1;
        UINT32 NoSecondarySidSupport : 1;
        UINT32 ParseAllEventData : 1;
        UINT32 StoppedShortPacketCapability : 1;
        UINT32 StoppedEdtlaCapability : 1;
        UINT32 ContiguousFrameIdCapability : 1;
        UINT32 MaxPrimaryStreamArraySize : 4;
        UINT32 XhciExtendedCapPointer : 16;
    } HccParams1;
    DWORD DoorbellOffset;
    DWORD RuntimeRegisterSpaceOffset;
    // Capability Parameter 2
    struct {
        DWORD U3EntryCapability : 1;
        DWORD Cmc : 1;
        DWORD ForceSaveContext : 1;
        DWORD Ctc : 1;
        DWORD Lec : 1;
        DWORD ConfigurationInformation : 1;
        DWORD ExtendedTbc : 1;
        DWORD ExtendedTbcTrbStatus : 1;
        DWORD Gsc : 1;
        DWORD VirtualizationBasedTrustedIoCapability : 1;
        DWORD Reserved : 22;
    } HccParams2;
    DWORD Vtiosoff; // Virtualization Based Trusted IO Register Space Offset
} XHCI_CAPABILITY_REGISTERS;

typedef struct _XHCI_OPERATIONAL_REGISTERS {
    struct {
        DWORD RunStop : 1;
        DWORD HcReset : 1;
        DWORD InterrupterEnable : 1;
        DWORD HostSystemErrorEnable : 1;
        DWORD LightHcReset : 1;
        DWORD ControllerSaveState : 1;
        DWORD ControllerRestoreState : 1;
        DWORD EnableWrapEvent : 1;
        DWORD EnableU3MfIndexStop : 1;
        DWORD Reserved0 : 3;
        DWORD CemEnable : 1;
        DWORD ExtendedTbcEnable : 1;
        DWORD ExtendedTbcTrbStatusEnable : 1;
        DWORD VtioEnale : 1;
        DWORD Reserved1 : 16;
    } UsbCommand;
    struct {
        DWORD HcHalted : 1;
        DWORD Reserved0 : 1;
        DWORD HostSystemError : 1;
        DWORD EventInterrupt : 1;
        DWORD PortChangeDetect : 1;
        DWORD Reserved1 : 3;
        DWORD SaveStateStatus : 1;
        DWORD RestoreStateStatus : 1;
        DWORD SaveRestoreError : 1;
        DWORD ControllerNotReady : 1;
        DWORD HcError : 1;
        DWORD Reserved3 : 19;
    } UsbStatus;
    DWORD PageSize;
    UINT64 Reserved0;
    struct {
        UINT16 NotificationEnable : 16; // Bit Mask N0-N15
        UINT16 Reserved : 16;
    } DeviceNotificationControl;
    struct {
        UINT64 RingCycleState : 1;
        UINT64 CommandStop : 1;
        UINT64 CommandAbort : 1;
        UINT64 CommandRingRunning : 1;
        UINT64 Reserved0 : 2;
        UINT64 CommandRingPointer : 58;
    } CommandRingControl;
    UINT8 Reserved1[0x10];
    // DCBAP (Must be 64 Byte Aligned)
    UINT64 DeviceContextBaseAddressArrayPointer;
    struct {
        DWORD MaxDeviceSlotsEnabled : 8;
        DWORD U3EntryEnable : 1;
        DWORD ConfigurationInformationEnable : 1;
        DWORD Reserved : 22;
    } Configure;
} XHCI_OPERATIONAL_REGISTER;

typedef struct _XHCI_PORT {
    struct {
        DWORD CurrentConnectStatus : 1;
        DWORD PortEnabled : 1;
        DWORD Reserved0 : 1;
        DWORD OverCurrentActive : 1;
        DWORD PortReset : 1;
        DWORD PortLinkState : 4;
        DWORD PortPower : 1;
        DWORD PortSpeed : 4;
        DWORD PortIndicatorControl : 2;
        DWORD PortLinkStateWriteStrobe : 1;
        DWORD ConnectStatusChange : 1;
        DWORD PortEnabledDisabledChange : 1;
        DWORD WarmPortResetChange : 1;
        DWORD OvercurrentChange : 1;
        DWORD PortResetChange : 1;
        DWORD PortLinkStateChange : 1;
        DWORD PortConfigErrorChange : 1;
        DWORD ColdAttachStatus : 1;
        DWORD WakeOnConnectEnable : 1;
        DWORD WakeOnDisconnectEnable : 1;
        DWORD WakeOnOvercurrentEnable : 1;
        DWORD Reserved1 : 2;
        DWORD DeviceRemovable : 1; // 0 = removable 1 = non-removable
        DWORD WarmPortReset : 1;
    } StatusControl;
    // USB3 Definition (All the remaining) (There is also USB2 Definition)
    struct {
        DWORD U1Timeout : 8;
        DWORD U2Timeout : 8;
        DWORD ForceLinkPmAccept : 1;
        DWORD Reserved : 15;
    } PowerManagementStatusControl;
    struct {
        DWORD LinkErrorCount : 16;
        DWORD RxLaneCount : 4;
        DWORD TxLaneCount : 4;
        DWORD Reserved : 8;
    } PortLinkInfo;
    struct {
        DWORD LinkSoftErrorCount : 16;
        DWORD Reserved : 16;
    } PortHardwareLpmControl;
} XHCI_PORT;

typedef struct _INTERRUPTER_REGISTERS {
    struct {
        DWORD InterruptPending : 1;
        DWORD InterruptEnable : 1;
        DWORD Reserved : 30;
    } InterruptManagement;
    struct {
        DWORD InterruptModerationInterval : 16;
        DWORD InterruptModerationCounter : 16;
    } InterruptModeration;
    DWORD EventRingSegmentTableSize;
    DWORD Reserved0;
    UINT64 EventRingSegmentTableAddress;
    UINT64 EventRingDequeuePointer;
} INTERRUPTER_REGISTERS;

typedef struct _XHCI_RUNTIME_REGISTERS {
    DWORD MicroframeIndex;
    DWORD Reserved0[0x1F - 4];
    INTERRUPTER_REGISTERS InterrupterRegisterSets[0x400];
} XHCI_RUNTIME_REGISTERS;

typedef struct _XHCI_DOORBELL_REGISTER {
    DWORD DoorbellTarget : 8;
    DWORD Reserved0 : 8;
    DWORD DoorbellStreamId : 16;
} XHCI_DOORBELL_REGISTER;

typedef struct _XHCI_DEVICE {
    RFDEVICE_OBJECT Device;
    XHCI_CAPABILITY_REGISTERS* CapabilityRegisters;
    XHCI_OPERATIONAL_REGISTER* OperationalRegisters;
} XHCI_DEVICE;