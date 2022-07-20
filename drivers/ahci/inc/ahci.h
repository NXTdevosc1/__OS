#pragma once
#include <kerneltypes.h>
#include <ata.h>
#pragma pack(push, 1)

typedef struct _HBA_REGISTERS {
    struct {
        DWORD NumPorts : 5; // Use Ports Implemented (This is the max ports supported by the HBA silicon)
        DWORD ExternalSataSupport : 1;
        DWORD EnclosureManagementSupported : 1;
        DWORD CommandCompletionCoalescingSupported : 1;
        DWORD NumCommandSlots : 5;
        DWORD PartialStateCapable : 1;
        DWORD SlumberStateCapable : 1;
        DWORD PioMultipleDrqBlock : 1;
        DWORD FisBasedSwitchingSupported : 1;
        DWORD SupportsPortMultiplier : 1;
        DWORD SupportsAhciModeOnly : 1;
        DWORD Reserved0 : 1;
        DWORD InterfaceSpeedSupport : 4; // 0=reserved, 1 = Gen1(1.5Gbps), 2=Gen2(3Gbps),3=Gen3(6Gbps)
        DWORD SupportsCommandListOverride : 1;
        DWORD SupportsActivityLed : 1;
        DWORD SupportsAggressiveLinkPowerManagement : 1;
        DWORD SupportsStaggeredSpinup : 1;
        DWORD SupportsMechanicalPresenceSwitch : 1;
        DWORD SupportsSnotificationRegister : 1;
        DWORD SupportsNativeCommandQueuing : 1;
        DWORD x64AddressingCapability : 1;
    } HostCapabilities;
    struct {
        DWORD HbaReset : 1;
        DWORD InterruptEnable : 1;
        DWORD MsiRevertToSingleMessage : 1;
        DWORD Reserved : 28;
        DWORD AhciEnable : 1;
    } GlobalHostControl;
    DWORD InterruptStatus;
    DWORD PortsImplemented;
    DWORD Version;
    struct {
        DWORD Enable : 1;
        DWORD Reserved0 : 2;
        DWORD Interrupt : 5;
        DWORD CommandCompletions : 8;
        DWORD TimeoutValue : 16;
    } CommandCompletionCoalescingControl;
    DWORD CommandCompletionCoalescingPorts;
    struct { 
        DWORD BufferSize : 16;
        DWORD Offset : 16;
    } EnclosureManagementLocation;
    struct {
        DWORD MessageReceived : 1;
        DWORD Reserved0 : 7;
        DWORD TransmitMessage : 1;
        DWORD Reset : 1;
        DWORD Reserved1 : 6;
        DWORD LedMessageTypes : 1;
        DWORD SAFTE_EnclosureManagementMessages : 1;
        DWORD SES2_EnclosureManagementMessages : 1;
        DWORD SGPIO_EnclosureManagementMessages : 1;
        DWORD Reserved2 : 4;
        DWORD SingleMessageBuffer : 1;
        DWORD TransmitOnly : 1;
        DWORD ActivityLedHardwareDriven : 1;
        DWORD PortMultiplierSupport : 1;
        DWORD Reserved3 : 4;
    } EnclosureManagementControl;
    struct {
        DWORD BiosOsHandoff : 1;
        DWORD NvmHciPresent : 1;
        DWORD AutomaticPartialToSlumberTransitions : 1;
        DWORD DeviceSleepSupport : 1;
        DWORD AggressiveDeviceSleepManagementSupport : 1;
        DWORD DeviceSleepEntranceFromSlumberOnly : 1;
        DWORD Reserved : 26;
    } ExtendedHostCapabilities;
    struct {
        DWORD BiosOwnedSemaphore : 1;
        DWORD OsOwnedSemaphore : 1;
        DWORD SmiOnOsOwnershipChangeEnable : 1;
        DWORD OsOwnershipChange : 1;
        DWORD BiosBusy : 1;
        DWORD Reserved : 27;
    } BiosOsHandoffControlAndStatus;
} HBA_REGISTERS;

typedef struct _HBA_PORT{
    // Splitted to low & high to use dword accesses
    DWORD CommandListBaseAddressLow; // Upper 32 Bit are not set if 64 Bit is not supported
    DWORD CommandListBaseAddressHigh; // Upper 32 Bit are not set if 64 Bit is not supported
    
    DWORD FisBaseAddressLow;
    DWORD FisBaseAddressHigh;
    struct {
        DWORD D2HRegisterFisInterrupt : 1;
        DWORD PioSetupFisInterrupt : 1;
        DWORD DmaSetupFisInterrupt : 1;
        DWORD SetDeviceBitsInterrupt : 1;
        DWORD UnknownFisInterrupt : 1;
        DWORD DescriptorProcessed : 1;
        DWORD PortConnectChangeStatus : 1;
        DWORD DeviceMechanicalPresenceStatus : 1;
        DWORD Reserved0 : 14;
        DWORD PhyRdyChangeStatus : 1;
        DWORD IncorrectPortMultiplierStatus : 1;
        DWORD OverflowStatus : 1;
        DWORD Reserved1 : 1;
        DWORD InterfaceNonFatalErrorStatus : 1;
        DWORD InterfaceFatalErrorStatus : 1;
        DWORD HostBusDataErrorStatus : 1;
        DWORD HostBusFatalErrorStatus : 1;
        DWORD TaskFileErrorStatus : 1;
        DWORD ColdPortDetectStatus : 1;
    } InterruptStatus;
    struct {
        DWORD D2HRegisterFisInterruptEnable : 1;
        DWORD PioSetupFisInterruptEnable : 1;
        DWORD DmaSetupFisInterruptEnable : 1;
        DWORD SetDeviceBitsFisInterruptEnable : 1;
        DWORD UnknownFisInterruptEnable : 1;
        DWORD DescriptorProcessedInterruptEnable : 1;
        DWORD PortChangeInterruptEnable : 1;
        DWORD DeviceMechanicalPresenceEnable : 1;
        DWORD Reserved0 : 14;
        DWORD PhyRdyChangeInterruptEnable : 1;
        DWORD IncorrectPortMultiplierEnable : 1;
        DWORD OverflowEnable : 1;
        DWORD Reserved1 : 1;
        DWORD InterfaceNonFatalErrorEnable : 1;
        DWORD InterfaceFatalErrorEnable : 1;
        DWORD HostBusDataErrorEnable : 1;
        DWORD HostBusFatalErrorEnable : 1;
        DWORD TaskFileErrorEnable : 1;
        DWORD ColdPresenceDetectEnable : 1;
    } InterruptEnable;
    struct {
        DWORD Start : 1;
        DWORD SpinupDevice : 1;
        DWORD PowerOnDevice : 1;
        DWORD CommandListOverride : 1;
        DWORD FisReceiveEnable : 1;
        DWORD Reserved0 : 3;
        DWORD CurrentCommandSlot : 5;
        DWORD MechanicalPresenceSwitchState : 1;
        DWORD FisReceiveRunning : 1;
        DWORD CommandListRunning : 1;
        DWORD ColdPresenceState : 1;
        DWORD PortMultiplierAttached : 1;
        DWORD HotPlugCapablePort : 1;
        DWORD MechanicalPresenceSwitchAttachedToPort : 1;
        DWORD ColdPresenceDetection : 1;
        DWORD ExternalSataPort : 1;
        DWORD FisBasedSwitchingCapablePort : 1;
        DWORD AutomaticPartialToSlumberTransitionsEnabled : 1;
        DWORD DeviceIsAtapi : 1;
        DWORD AggressiveLinkPowerManagementEnable : 1;
        DWORD AggressiveSlumberPartial : 1;
        DWORD InterfaceCommunicationControl : 5;
    } CommandStatus;
    DWORD Reserved0;
    struct {
        DWORD Error : 1;
        DWORD CommandSpecific0 : 2;
        DWORD DataTransferRequested : 1;
        DWORD CommandSpecific1 : 3;
        DWORD Busy : 1;
        DWORD TaskFileErrorCopy : 8;
        DWORD Reserved0 : 16;
    } TaskFileData;
    struct {
        DWORD SectorCount : 8;
        DWORD LbaLow : 8;
        DWORD LbaMid : 8;
        DWORD LbaHigh : 8;
    } PortSignature;
    struct {
        DWORD DeviceDetection : 4; // 3/1 = Device Present, 0 = Device not present
        DWORD CurrentInterfaceSpeed : 4;
        DWORD InterfacePowerManagement : 4;
        DWORD Reserved0 : 20;
    } SataStatus;

    struct {
        DWORD DeviceDetectionInitialization : 4;
        DWORD SpeedAllowed : 4;
        DWORD InterfacePowerManagementTransitionsAllowed : 4;
        DWORD SelectPowerManagement : 4;
        DWORD PortMultiplierPort : 4;
        DWORD Reserved0 : 12;
    } SataControl;
    struct {
        DWORD RecoveredDataIntegrityError : 1;
        DWORD RecoveredCommunicationsError : 1;
        DWORD Reserved : 6;
        DWORD TransientDataIntegrityError : 1;
        DWORD PersistentCommunicationOrDataIntegrityError : 1;
        DWORD ProtocolError : 1;
        DWORD InternalError : 1;
        DWORD Reserved0 : 4;
        // DIAGNOSTICS
        DWORD PhyRdyChange : 1;
        DWORD PhyInternalError : 1;
        DWORD CommWake : 1;
        DWORD DecodeError10Bto8B : 1;
        DWORD DisparityError : 1;
        DWORD CRCError : 1;
        DWORD HandshakeError : 1;
        DWORD LinkSequenceError : 1;
        DWORD TransportStateTransitionError : 1;
        DWORD UnknownFisType : 1;
        DWORD Exchanged : 1;
        DWORD Reserved1 : 5;
    } SataError;
    DWORD SataActive;
    DWORD CommandIssue;
    DWORD SataNotification;
    struct {
        DWORD Enable : 1;
        DWORD DeviceErrorClear : 1;
        DWORD SingleDeviceError : 1;
        DWORD Reserved0 : 5;
        DWORD DeviceToIssue : 4;
        DWORD ActiveDeviceOptimization : 4;
        DWORD DeviceWithError : 4;
        DWORD Reserved1 : 12;
    } FisBasedSwitchingControl;
    struct {
        DWORD AggressiveDeviceSleepEnable : 1;
        DWORD DeviceSleepPresent : 1;
        DWORD DeviceSleepExitTimeout : 8;
        DWORD MinimumDeviceSleepAssertionTime : 5;
        DWORD DeviceSleepIdleTimeout : 10;
        DWORD DitoMultiplier : 4;
        DWORD Reserved0 : 3;
    } DeviceSleep;
    char Reserved[40];
    char VendorSpecific[0x10];
} HBA_PORT;

typedef struct _AHCI_COMMAND_LIST_ENTRY {
    // Description Information
    DWORD CommandFisLength : 5;
    DWORD Atapi : 1;
    DWORD Write : 1;
    DWORD Prefetchable : 1;
    DWORD Reset : 1;
    DWORD BIST : 1; // Built-in Self Test
    DWORD ClearBusyUponROK : 1;
    DWORD Reserved0 : 1;
    DWORD PortMultiplierPort : 4;
    DWORD PrdtLength : 16;
    // Command Status
    DWORD PrdtByteCount;
    // Command Table Base Address (must be 128 byte aligned)
    UINT64 CommandTableAddress;
    // Reserved
    DWORD Reserved1[4];
} AHCI_COMMAND_LIST_ENTRY;

#define NUM_PRDT_PER_CMDTBL 0x20 
#define MAX_PRDT_BYTE_COUNT 0x400000
typedef struct _PHYSICAL_REGION_DESCRIPTOR_TABLE {
    // Data Base Address (2 Byte aligned [Bit 0 is reserved])
    UINT64 DataBaseAddress;
    DWORD Reserved0;
    // Description Information
    DWORD DataByteCount : 22;
    DWORD Reserved1 : 9;
    DWORD InterruptOnCompletion : 1;
} PHYSICAL_REGION_DESCRIPTOR_TABLE;

typedef struct _AHCI_COMMAND_TABLE {
    UINT8 CommandFis[0x40];
    UINT8 AtapiCommand[0x10];
    UINT8 Reserved0[0x30];
    PHYSICAL_REGION_DESCRIPTOR_TABLE Prdt[NUM_PRDT_PER_CMDTBL];
} AHCI_COMMAND_TABLE;












typedef struct _AHCI_RECEVIED_FIS {
    ATA_FIS_DMA_SETUP DmaSetup;
    UINT32 Reserved0;
    ATA_FIS_PIO_SETUP PioSetup;
    UINT8 Reserved1[12];
    ATA_FIS_D2H D2h;
    UINT32 Reserved2;
    UINT64 SetDeviceBitsFis;
    UINT8 UnknownFis[0x40];
    UINT8 Reserved[0xFF - 0xA0];
} AHCI_RECEIVED_FIS;

#pragma pack(pop)

#define AHCI_PORTS_OFFSET 0x100
#define AHCI_MAX_PORTS 0x20
#define AHCI_CONFIGURATION_PAGES 2

typedef struct _AHCI_DEVICE AHCI_DEVICE,*RFAHCI_DEVICE;

typedef struct _AHCI_DEVICE_PORT {
    RFAHCI_DEVICE Ahci;
    BOOL DeviceDetected;
    RFDEVICE_OBJECT Controller;
    RFDEVICE_OBJECT Device; // Related device to this port
    UINT8 PortIndex;
    HBA_PORT* Port;
    AHCI_COMMAND_LIST_ENTRY* CommandList;
    AHCI_COMMAND_TABLE* CommandTables;
    AHCI_RECEIVED_FIS* ReceivedFis;
    DWORD CommandListMask;
    char* AllocatedBuffer;
    RFTHREAD PendingCommands[0x20];
    UINT32 DoneCommands;
    UINT32 UsedCommandSlots;
} AHCI_DEVICE_PORT, *RFAHCI_DEVICE_PORT;

typedef struct _AHCI_DEVICE {
    RFDEVICE_OBJECT Device;
    HBA_REGISTERS* Hba;
    HBA_PORT* HbaPorts;
    UINT NumPorts; // Num Implemented Ports
    UINT MaxCommandSlots;
    // Implemented Ports
    AHCI_DEVICE_PORT Ports[AHCI_MAX_PORTS];
} AHCI_DEVICE, *RFAHCI_DEVICE;

#define AHCI_DEVICE_HOST 0
#define AHCI_DEVICE_LBA 0x40

typedef struct _AHCI_COMMAND_ADDRESS AHCI_COMMAND_ADDRESS;



void AhciReset(RFAHCI_DEVICE Ahci);
void AhciInitializePort(RFAHCI_DEVICE_PORT Port);
KERNELSTATUS AhciHostToDevice(RFAHCI_DEVICE_PORT Port, UINT64 Lba, UINT8 Command, UINT8 Device, UINT16 Count, UINT16 NumCommandAddressDescriptors, AHCI_COMMAND_ADDRESS* CommandAddresses);
UINT32 AhciAllocateCommand(RFAHCI_DEVICE_PORT Port);

int AhciSataComReset(HBA_PORT* HbaPort);

// returns Task file error copy if Error
int AhciIssueCommand(RFAHCI_DEVICE_PORT Port, UINT CommandIndex);