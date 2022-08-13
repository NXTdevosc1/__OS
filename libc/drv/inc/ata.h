#pragma once
#include <kerneltypes.h>

typedef enum _FIS_TYPE {
    FIS_TYPE_H2D = 0x27,
    FIS_TYPE_D2H = 0x34,
    FIS_TYPE_DMA_ACTIVATE = 0x39,
    FIS_TYPE_DMA_SETUP = 0x41,
    FIS_TYPE_DATA = 0x46,
    FIS_TYPE_BIST = 0x58,
    FIS_TYPE_PIO_SETUP = 0x5F,
    FIS_TYPE_SET_DEVICE_BITS = 0xA1
} FIS_TYPE;


#pragma pack(push, 1)


typedef struct _ATA_FIS_H2D {
    UINT8 FisType;
    UINT8 PortMultiplierPort : 4;
    UINT8 Reserved0 : 3;
    UINT8 CommandControl : 1; // 1 = Command, 0 = Control
    UINT8 Command;
    UINT8 FeatureLow;
    UINT16 Lba0;
    UINT8 Lba1;
    UINT8 Device; // 1 << 6 = LBA Mode
    UINT16 Lba2;
    UINT8 Lba3;
    UINT8 FeatureHigh;
    UINT16 Count;
    UINT8 IsochronousCommandCompletion;
    UINT8 Control;
    UINT8 Reserved1[4];
} ATA_FIS_H2D;

typedef struct _ATA_FIS_D2H {
    UINT8 FisType;
    UINT8 PortMultiplier : 4;
    UINT8 Reserved0 : 2;
    UINT8 Interrupt : 1;
    UINT8 Reserved1 : 1;
    UINT8 Status;
    UINT8 Error;

    UINT8 Lba0, Lba1, Lba2, Device;
    UINT8 Lba3, Lba4, Lba5, Reserved2;
    UINT16 Count;
    UINT8 Reserved3[2];
    DWORD Reserved4;
} ATA_FIS_D2H;

typedef struct _ATA_FIS_DMA_SETUP {
    UINT8 FisType;
    UINT8 PortMultiplier : 4;
    UINT8 Reserved0 : 1;
    UINT8 Direction : 1;
    UINT8 Interrupt : 1;
    UINT8 AutoActivate : 1;
    UINT16 Reserved2;
    UINT64 DmaBufferId;
    UINT32 Reserved3;
    UINT32 DmaBufferOffset;
    UINT32 TransferCount;
    UINT32 Reserved4;
} ATA_FIS_DMA_SETUP;

typedef struct _ATA_FIS_PIO_SETUP {
    UINT8 FisType;
    UINT8 PortMultiplier : 4;
    UINT8 Reserved0 : 1;
    UINT8 Direction : 1;
    UINT8 Interrupt : 1;
    UINT8 Reserved1 : 1;
    UINT8 Status, Error;
    UINT8 Lba0, Lba1, Lba2, Device;
    UINT8 Lba3, Lba4, Lba5, Reserved2;
    UINT16 Count;
    UINT8 Reserved3;
    UINT8 eStatus;
    UINT16 TransferCount;
    UINT16 Reserved4;
} ATA_FIS_PIO_SETUP;

typedef struct _ATA_FIS_DATA {
    UINT8 FisType;
    UINT8 PortMultiplier : 4;
    UINT8 Reserved0 : 4;
    UINT8 Reserved1[2];
    UINT32 Data[];
} ATA_FIS_DATA;


// ATA IDENTIFY DEVICE (From ata.h win32)

typedef struct _ATA_IDENTIFY_DEVICE_DATA {
  struct {
    UINT16 Reserved1 : 1;
    UINT16 Retired3 : 1;
    UINT16 ResponseIncomplete : 1;
    UINT16 Retired2 : 3;
    UINT16 FixedDevice : 1;
    UINT16 RemovableMedia : 1;
    UINT16 Retired1 : 7;
    UINT16 DeviceType : 1;
  } GeneralConfiguration;
  UINT16 NumCylinders;
  UINT16 SpecificConfiguration;
  UINT16 NumHeads;
  UINT16 Retired1[2];
  UINT16 NumSectorsPerTrack;
  UINT16 VendorUnique1[3];
  UINT8  SerialNumber[20];
  UINT16 Retired2[2];
  UINT16 Obsolete1;
  UINT8  FirmwareRevision[8];
  UINT8  ModelNumber[40];
  UINT8  MaximumBlockTransfer;
  UINT8  VendorUnique2;
  struct {
    UINT16 FeatureSupported : 1;
    UINT16 Reserved : 15;
  } TrustedComputing;
  struct {
    UINT8  CurrentLongPhysicalSectorAlignment : 2;
    UINT8  ReservedByte49 : 6;
    UINT8  DmaSupported : 1;
    UINT8  LbaSupported : 1;
    UINT8  IordyDisable : 1;
    UINT8  IordySupported : 1;
    UINT8  Reserved1 : 1;
    UINT8  StandybyTimerSupport : 1;
    UINT8  Reserved2 : 2;
    UINT16 ReservedWord50;
  } Capabilities;
  UINT16 ObsoleteWords51[2];
  UINT16 TranslationFieldsValid : 3;
  UINT16 Reserved3 : 5;
  UINT16 FreeFallControlSensitivity : 8;
  UINT16 NumberOfCurrentCylinders;
  UINT16 NumberOfCurrentHeads;
  UINT16 CurrentSectorsPerTrack;
  UINT32  CurrentSectorCapacity;
  UINT8  CurrentMultiSectorSetting;
  UINT8  MultiSectorSettingValid : 1;
  UINT8  ReservedByte59 : 3;
  UINT8  SanitizeFeatureSupported : 1;
  UINT8  CryptoScrambleExtCommandSupported : 1;
  UINT8  OverwriteExtCommandSupported : 1;
  UINT8  BlockEraseExtCommandSupported : 1;
  UINT32  UserAddressableSectors;
  UINT16 ObsoleteWord62;
  UINT16 MultiWordDMASupport : 8;
  UINT16 MultiWordDMAActive : 8;
  UINT16 AdvancedPIOModes : 8;
  UINT16 ReservedByte64 : 8;
  UINT16 MinimumMWXferCycleTime;
  UINT16 RecommendedMWXferCycleTime;
  UINT16 MinimumPIOCycleTime;
  UINT16 MinimumPIOCycleTimeIORDY;
  struct {
    UINT16 ZonedCapabilities : 2;
    UINT16 NonVolatileWriteCache : 1;
    UINT16 ExtendedUserAddressableSectorsSupported : 1;
    UINT16 DeviceEncryptsAllUserData : 1;
    UINT16 ReadZeroAfterTrimSupported : 1;
    UINT16 Optional28BitCommandsSupported : 1;
    UINT16 IEEE1667 : 1;
    UINT16 DownloadMicrocodeDmaSupported : 1;
    UINT16 SetMaxSetPasswordUnlockDmaSupported : 1;
    UINT16 WriteBufferDmaSupported : 1;
    UINT16 ReadBufferDmaSupported : 1;
    UINT16 DeviceConfigIdentifySetDmaSupported : 1;
    UINT16 LPSAERCSupported : 1;
    UINT16 DeterministicReadAfterTrimSupported : 1;
    UINT16 CFastSpecSupported : 1;
  } AdditionalSupported;
  UINT16 ReservedWords70[5];
  UINT16 QueueDepth : 5;
  UINT16 ReservedWord75 : 11;
  struct {
    UINT16 Reserved0 : 1;
    UINT16 SataGen1 : 1;
    UINT16 SataGen2 : 1;
    UINT16 SataGen3 : 1;
    UINT16 Reserved1 : 4;
    UINT16 NCQ : 1;
    UINT16 HIPM : 1;
    UINT16 PhyEvents : 1;
    UINT16 NcqUnload : 1;
    UINT16 NcqPriority : 1;
    UINT16 HostAutoPS : 1;
    UINT16 DeviceAutoPS : 1;
    UINT16 ReadLogDMA : 1;
    UINT16 Reserved2 : 1;
    UINT16 CurrentSpeed : 3;
    UINT16 NcqStreaming : 1;
    UINT16 NcqQueueMgmt : 1;
    UINT16 NcqReceiveSend : 1;
    UINT16 DEVSLPtoReducedPwrState : 1;
    UINT16 Reserved3 : 8;
  } SerialAtaCapabilities;
  struct {
    UINT16 Reserved0 : 1;
    UINT16 NonZeroOffsets : 1;
    UINT16 DmaSetupAutoActivate : 1;
    UINT16 DIPM : 1;
    UINT16 InOrderData : 1;
    UINT16 HardwareFeatureControl : 1;
    UINT16 SoftwareSettingsPreservation : 1;
    UINT16 NCQAutosense : 1;
    UINT16 DEVSLP : 1;
    UINT16 HybridInformation : 1;
    UINT16 Reserved1 : 6;
  } SerialAtaFeaturesSupported;
  struct {
    UINT16 Reserved0 : 1;
    UINT16 NonZeroOffsets : 1;
    UINT16 DmaSetupAutoActivate : 1;
    UINT16 DIPM : 1;
    UINT16 InOrderData : 1;
    UINT16 HardwareFeatureControl : 1;
    UINT16 SoftwareSettingsPreservation : 1;
    UINT16 DeviceAutoPS : 1;
    UINT16 DEVSLP : 1;
    UINT16 HybridInformation : 1;
    UINT16 Reserved1 : 6;
  } SerialAtaFeaturesEnabled;
  UINT16 MajorRevision;
  UINT16 MinorRevision;
  struct {
    UINT16 SmartCommands : 1;
    UINT16 SecurityMode : 1;
    UINT16 RemovableMediaFeature : 1;
    UINT16 PowerManagement : 1;
    UINT16 Reserved1 : 1;
    UINT16 WriteCache : 1;
    UINT16 LookAhead : 1;
    UINT16 ReleaseInterrupt : 1;
    UINT16 ServiceInterrupt : 1;
    UINT16 DeviceReset : 1;
    UINT16 HostProtectedArea : 1;
    UINT16 Obsolete1 : 1;
    UINT16 WriteBuffer : 1;
    UINT16 ReadBuffer : 1;
    UINT16 Nop : 1;
    UINT16 Obsolete2 : 1;
    UINT16 DownloadMicrocode : 1;
    UINT16 DmaQueued : 1;
    UINT16 Cfa : 1;
    UINT16 AdvancedPm : 1;
    UINT16 Msn : 1;
    UINT16 PowerUpInStandby : 1;
    UINT16 ManualPowerUp : 1;
    UINT16 Reserved2 : 1;
    UINT16 SetMax : 1;
    UINT16 Acoustics : 1;
    UINT16 BigLba : 1;
    UINT16 DeviceConfigOverlay : 1;
    UINT16 FlushCache : 1;
    UINT16 FlushCacheExt : 1;
    UINT16 WordValid83 : 2;
    UINT16 SmartErrorLog : 1;
    UINT16 SmartSelfTest : 1;
    UINT16 MediaSerialNumber : 1;
    UINT16 MediaCardPassThrough : 1;
    UINT16 StreamingFeature : 1;
    UINT16 GpLogging : 1;
    UINT16 WriteFua : 1;
    UINT16 WriteQueuedFua : 1;
    UINT16 WWN64Bit : 1;
    UINT16 URGReadStream : 1;
    UINT16 URGWriteStream : 1;
    UINT16 ReservedForTechReport : 2;
    UINT16 IdleWithUnloadFeature : 1;
    UINT16 WordValid : 2;
  } CommandSetSupport;
  struct {
    UINT16 SmartCommands : 1;
    UINT16 SecurityMode : 1;
    UINT16 RemovableMediaFeature : 1;
    UINT16 PowerManagement : 1;
    UINT16 Reserved1 : 1;
    UINT16 WriteCache : 1;
    UINT16 LookAhead : 1;
    UINT16 ReleaseInterrupt : 1;
    UINT16 ServiceInterrupt : 1;
    UINT16 DeviceReset : 1;
    UINT16 HostProtectedArea : 1;
    UINT16 Obsolete1 : 1;
    UINT16 WriteBuffer : 1;
    UINT16 ReadBuffer : 1;
    UINT16 Nop : 1;
    UINT16 Obsolete2 : 1;
    UINT16 DownloadMicrocode : 1;
    UINT16 DmaQueued : 1;
    UINT16 Cfa : 1;
    UINT16 AdvancedPm : 1;
    UINT16 Msn : 1;
    UINT16 PowerUpInStandby : 1;
    UINT16 ManualPowerUp : 1;
    UINT16 Reserved2 : 1;
    UINT16 SetMax : 1;
    UINT16 Acoustics : 1;
    UINT16 BigLba : 1;
    UINT16 DeviceConfigOverlay : 1;
    UINT16 FlushCache : 1;
    UINT16 FlushCacheExt : 1;
    UINT16 Resrved3 : 1;
    UINT16 Words119_120Valid : 1;
    UINT16 SmartErrorLog : 1;
    UINT16 SmartSelfTest : 1;
    UINT16 MediaSerialNumber : 1;
    UINT16 MediaCardPassThrough : 1;
    UINT16 StreamingFeature : 1;
    UINT16 GpLogging : 1;
    UINT16 WriteFua : 1;
    UINT16 WriteQueuedFua : 1;
    UINT16 WWN64Bit : 1;
    UINT16 URGReadStream : 1;
    UINT16 URGWriteStream : 1;
    UINT16 ReservedForTechReport : 2;
    UINT16 IdleWithUnloadFeature : 1;
    UINT16 Reserved4 : 2;
  } CommandSetActive;
  UINT16 UltraDMASupport : 8;
  UINT16 UltraDMAActive : 8;
  struct {
    UINT16 TimeRequired : 15;
    UINT16 ExtendedTimeReported : 1;
  } NormalSecurityEraseUnit;
  struct {
    UINT16 TimeRequired : 15;
    UINT16 ExtendedTimeReported : 1;
  } EnhancedSecurityEraseUnit;
  UINT16 CurrentAPMLevel : 8;
  UINT16 ReservedWord91 : 8;
  UINT16 MasterPasswordID;
  UINT16 HardwareResetResult;
  UINT16 CurrentAcousticValue : 8;
  UINT16 RecommendedAcousticValue : 8;
  UINT16 StreamMinRequestSize;
  UINT16 StreamingTransferTimeDMA;
  UINT16 StreamingAccessLatencyDMAPIO;
  UINT32  StreamingPerfGranularity;
  UINT64  Max48BitLBA;
  UINT16 StreamingTransferTime;
  UINT16 DsmCap;
  struct {
    UINT16 LogicalSectorsPerPhysicalSector : 4;
    UINT16 Reserved0 : 8;
    UINT16 LogicalSectorLongerThan256Words : 1;
    UINT16 MultipleLogicalSectorsPerPhysicalSector : 1;
    UINT16 Reserved1 : 2;
  } PhysicalLogicalSectorSize;
  UINT16 InterSeekDelay;
  UINT16 WorldWideName[4];
  UINT16 ReservedForWorldWideName128[4];
  UINT16 ReservedForTlcTechnicalReport;
  UINT16 WordsPerLogicalSector[2];
  struct {
    UINT16 ReservedForDrqTechnicalReport : 1;
    UINT16 WriteReadVerify : 1;
    UINT16 WriteUncorrectableExt : 1;
    UINT16 ReadWriteLogDmaExt : 1;
    UINT16 DownloadMicrocodeMode3 : 1;
    UINT16 FreefallControl : 1;
    UINT16 SenseDataReporting : 1;
    UINT16 ExtendedPowerConditions : 1;
    UINT16 Reserved0 : 6;
    UINT16 WordValid : 2;
  } CommandSetSupportExt;
  struct {
    UINT16 ReservedForDrqTechnicalReport : 1;
    UINT16 WriteReadVerify : 1;
    UINT16 WriteUncorrectableExt : 1;
    UINT16 ReadWriteLogDmaExt : 1;
    UINT16 DownloadMicrocodeMode3 : 1;
    UINT16 FreefallControl : 1;
    UINT16 SenseDataReporting : 1;
    UINT16 ExtendedPowerConditions : 1;
    UINT16 Reserved0 : 6;
    UINT16 Reserved1 : 2;
  } CommandSetActiveExt;
  UINT16 ReservedForExpandedSupportandActive[6];
  UINT16 MsnSupport : 2;
  UINT16 ReservedWord127 : 14;
  struct {
    UINT16 SecuritySupported : 1;
    UINT16 SecurityEnabled : 1;
    UINT16 SecurityLocked : 1;
    UINT16 SecurityFrozen : 1;
    UINT16 SecurityCountExpired : 1;
    UINT16 EnhancedSecurityEraseSupported : 1;
    UINT16 Reserved0 : 2;
    UINT16 SecurityLevel : 1;
    UINT16 Reserved1 : 7;
  } SecurityStatus;
  UINT16 ReservedWord129[31];
  struct {
    UINT16 MaximumCurrentInMA : 12;
    UINT16 CfaPowerMode1Disabled : 1;
    UINT16 CfaPowerMode1Required : 1;
    UINT16 Reserved0 : 1;
    UINT16 Word160Supported : 1;
  } CfaPowerMode1;
  UINT16 ReservedForCfaWord161[7];
  UINT16 NominalFormFactor : 4;
  UINT16 ReservedWord168 : 12;
  struct {
    UINT16 SupportsTrim : 1;
    UINT16 Reserved0 : 15;
  } DataSetManagementFeature;
  UINT16 AdditionalProductID[4];
  UINT16 ReservedForCfaWord174[2];
  UINT16 CurrentMediaSerialNumber[30];
  struct {
    UINT16 Supported : 1;
    UINT16 Reserved0 : 1;
    UINT16 WriteSameSuported : 1;
    UINT16 ErrorRecoveryControlSupported : 1;
    UINT16 FeatureControlSuported : 1;
    UINT16 DataTablesSuported : 1;
    UINT16 Reserved1 : 6;
    UINT16 VendorSpecific : 4;
  } SCTCommandTransport;
  UINT16 ReservedWord207[2];
  struct {
    UINT16 AlignmentOfLogicalWithinPhysical : 14;
    UINT16 Word209Supported : 1;
    UINT16 Reserved0 : 1;
  } BlockAlignment;
  UINT16 WriteReadVerifySectorCountMode3Only[2];
  UINT16 WriteReadVerifySectorCountMode2Only[2];
  struct {
    UINT16 NVCachePowerModeEnabled : 1;
    UINT16 Reserved0 : 3;
    UINT16 NVCacheFeatureSetEnabled : 1;
    UINT16 Reserved1 : 3;
    UINT16 NVCachePowerModeVersion : 4;
    UINT16 NVCacheFeatureSetVersion : 4;
  } NVCacheCapabilities;
  UINT16 NVCacheSizeLSW;
  UINT16 NVCacheSizeMSW;
  UINT16 NominalMediaRotationRate;
  UINT16 ReservedWord218;
  struct {
    UINT8 NVCacheEstimatedTimeToSpinUpInSeconds;
    UINT8 Reserved;
  } NVCacheOptions;
  UINT16 WriteReadVerifySectorCountMode : 8;
  UINT16 ReservedWord220 : 8;
  UINT16 ReservedWord221;
  struct {
    UINT16 MajorVersion : 12;
    UINT16 TransportType : 4;
  } TransportMajorVersion;
  UINT16 TransportMinorVersion;
  UINT16 ReservedWord224[6];
  UINT64  ExtendedNumberOfUserAddressableSectors;
  UINT16 MinBlocksPerDownloadMicrocodeMode03;
  UINT16 MaxBlocksPerDownloadMicrocodeMode03;
  UINT16 ReservedWord236[19];
  UINT16 Signature : 8;
  UINT16 CheckSum : 8;
} ATA_IDENTIFY_DEVICE_DATA, *RFATA_IDENTIFY_DEVICE_DATA;

#pragma pack(pop)
