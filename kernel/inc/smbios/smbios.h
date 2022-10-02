#pragma once
#include <krnltypes.h>

char* SMBIOS_SIGNATURE;
char* SMBIOS_INTERMEDIATE_ANCHOR_STRING;

#define SMBIOS_SIGNATURE_LENGTH 4

#pragma pack(push, 1)

enum SMBIOS_STRUCTURE_TYPE {
	SMBIOS_BIOS_INFORMATION = 0,
	SMBIOS_SYSTEM_INFORMATION = 1,
	SMBIOS_BASE_BOARD_INFORMATION = 2,
	SMBIOS_SYSTEM_ENCLOSURE = 3,
	SMBIOS_PROCESSOR_INFORMATION = 4,
	SMBIOS_CACHE_INFORMATION = 7,
	SMBIOS_SYSTEM_SLOTS = 9,
	SMBIOS_PHYSICAL_MEMORY_ARRAY = 16,
	SMBIOS_MEMORY_DEVICE = 17,
	SMBIOS_MEMORY_ARRAY_MAPPED_ADDRESS = 19,
	SMBIOS_SYSTEM_BOOT_INFORMATION = 32,
	SMBIOS_ENDOF_TABLE = 0x7F
};

typedef struct _SMBIOS_ENTRY_POINT{
	UCHAR Signature[4];
	UCHAR CheckSum;
	UCHAR EntryPointLength;
	UCHAR MajorVersion;
	UCHAR MinorVersion;
	WORD MaxStructureSize;
	UCHAR Revision;
	UCHAR FormattedArea[5];
	UCHAR IntermediateAnchorString[5]; // _DMI_
	UCHAR IntermediateCheckSum;
	WORD StructureTableLength; // Length of SMBIOS Structure Table
	DWORD StructureTableAddress;
	WORD NumSmbiosStructures;
	UCHAR SmbiosBcdRevision;
} SMBIOS_ENTRY_POINT;

typedef struct _SMBIOS_STRUCTURE_HEADER {
	UCHAR Type;
	UCHAR Length;
	WORD Handle;
} SMBIOS_STRUCTURE_HEADER;

// TYPE 0
typedef struct _SMBIOS_BIOS_INFORMATION_STRUCTURE {
	SMBIOS_STRUCTURE_HEADER Header;
	UCHAR Vendor;
	UCHAR BiosVersion;
	WORD BiosStartingAddressSegment;
	UCHAR BiosReleaseDate;
	UCHAR BiosRomSize;
	QWORD BiosCharacteristics;
	UCHAR BiosCharacteristicsExtensionBytes1;
	UCHAR BiosCharacteristicsExtensionBytes2;
	UCHAR SystemBiosMajorRelease;
	UCHAR SystemBiosMinorRelease;
	UCHAR EmbeddedControllerFirmwareMajorRelease;
	UCHAR EmbeddedControllerFirmwareMinorRelease;
} SMBIOS_BIOS_INFORMATION_STRUCTURE;

// TYPE 1

typedef struct _SMBIOS_UUID {
	DWORD TimeLow;
	WORD  TimeMid;
	BYTE  TimeHigh;
	BYTE  Version;
	BYTE  ClockSequenceHigh : 4;
	BYTE  Reserved : 4;
	BYTE  ClockSequenceLow;
	UCHAR  Node[6];
} SMBIOS_UUID;

typedef struct _SMBIOS_SYSTEM_INFORMATION_STRUCTURE {
	SMBIOS_STRUCTURE_HEADER Header;
	UCHAR Manufacturer;
	UCHAR ProductName;
	UCHAR Version;
	UCHAR SerialNumber;
	SMBIOS_UUID UUID;
	UCHAR WakeupType;
	UCHAR SkuNumber;
	UCHAR Family;
} SMBIOS_SYSTEM_INFORMATION_STRUCTURE;

enum SMBIOS_SYSTEM_WAKEUP_TYPE {
	SYSTEM_WAKEUP_RESERVED = 0,
	SYSTEM_WAKEUP_OTHER = 1,
	SYSTEM_WAKEUP_UNKNOWN = 2,
	SYSTEM_WAKEUP_APM_TIMER = 3,
	SYSTEM_WAKEUP_MODERN_RING = 4,
	SYSTEM_WAKEUP_LAN_REMOTE = 5,
	SYSTEM_WAKEUP_POWER_SWITCH = 6,
	SYSTEM_WAKEUP_PCI_PME = 7,
	SYSTEM_WAKEUP_AC_POWER_RESTORED = 8
};

enum BIOS_CHARACTERISTICS_EXTENSION_BYTE1 {
	BIOS_ACPI_SUPPORTED = 1,
	BIOS_USB_LEGACY_SUPPORTED = 2,
	BIOS_AGP_SUPPORTED = 4,
	BIOS_I2O_BOOT_SUPPORTED = 8,
	BIOS_LS120_BOOT_SUPPORTED = 0x10,
	BIOS_ATAPI_ZIP_DRIVE_BOOT_SUPPORTED = 0x20,
	BIOS_1394_BOOT_SUPPORTED = 0x40,
	BIOS_SMART_BATTERY_SUPPORTED = 0x80
};

enum BIOS_CHARACTERISTICS_EXTENSION_BYTE2 {
	BIOS_BOOT_SPECIFICATION_SUPPORTED = 1,
	BIOS_FUNCTION_KEY_INITIALIZED_NETWORK_SERVICE_BOOT_SUPPORTED = 2,
	BIOS_ENABLE_TARGETED_CONTENT_DISTRIBUTION = 4
};

typedef struct _SMBIOS_BASE_BOARD_INFORMATION_STRUCTURE {
	SMBIOS_STRUCTURE_HEADER Header;
	UCHAR Manufacturer;
	UCHAR Product;
	UCHAR Version;
	UCHAR SerialNumber;
	UCHAR AssetTag;
	UCHAR FeatureFlags;
	UCHAR LocationInChassis;
	WORD ChassisHandle;
	UCHAR BoardType;
	UCHAR NumberOfContainedObjectHandles;
	WORD ContainedObjectHandles[];
} SMBIOS_BASE_BOARD_INFORMATION_STRUCTURE;

typedef struct _SMBIOS_PROCESSOR_INFORMATION_STRUCTURE{
	SMBIOS_STRUCTURE_HEADER Header;
	UCHAR  SocketDesignation;
	 /* Processor Type Field:
	  * 1 = Other
	  * 2 = Unknown
	  * 3 = Central Processor
	  * 4 = Math Processor
	  * 5 = DSP Processor
	  * 6 = Video Processor
	  */
	UINT8 ProcessorType; 
	UINT8 ProcessorFamily;
	UCHAR  ProcessorManufacturer;
	QWORD ProcessorId;
	UCHAR  ProcessorVersion;
	 /* Voltage Capability Field
	  * Bit 0 = 5V
	  * Bit 1 = 3.3V
	  * Bit 2 = 2.9V
	  * Bit 3 = Reserved
	  * Note : Setting of multiple bits indicates the socket is configurable
	  */
	UINT8 Voltage;
	WORD  ExternalClock; // Bus Frequency
	WORD  MaxSpeed; // In MHz
	WORD  CurrentSpeed; // In MHz
	struct {
		 /* 0 = unknown, 1 = cpu enabled
		  * 2 = cpu disabled by user through bios setup
		  * 3 = cpu disabled by bios (POST Error), 
		  * 4 = cpu is idle (waiting to be enabled)
		  * 5-6 Reserved
		  * 7 Other
		  */
	UINT8 CpuStatus : 3;
	UINT8 Reserved0 : 3;
	UINT8 CpuSocketPopulated : 1;
	UINT8 Reserved1 : 1;
	} Status;
	UINT8 ProcessorUpgrade;
	WORD  L1CacheHandle; // 0xFFFF If the processor has no L1 Cache
	WORD  L2CacheHandle; // 0xFFFF If the processor has no L2 Cache
	WORD  L3CacheHandle; // 0xFFFF If the processor has no L3 Cache
	UCHAR SerialNumber;
	UCHAR AssetTag;
	UCHAR PartNumber;
	UINT8 CoreCount; // if 0xFF Then read from CoreCount2
	UINT8 CoreEnabled; // Num cores enabled per processor socket
	UINT8 ThreadCount; // Num threads per processor socket
	WORD  ProcessorCharacteristics;
	WORD  ProcessorFamily2;
	WORD  CoreCount2;
	WORD  CoreEnabled2;
	WORD  ThreadCount2;
} SMBIOS_PROCESSOR_INFORMATION_STRUCTURE;

#pragma pack(pop)

// Gets the preferred version of SMBIOS
void SystemManagementBiosInitialize(void* _SmBIOSHeader);
UCHAR SmbiosGetMajorVersion(void);
UCHAR SmbiosGetMinorVersion(void);
WORD SmbiosGetStructuresCount(void);
SMBIOS_STRUCTURE_HEADER* SmbiosGetStructure(WORD StructureIndex);
SMBIOS_BIOS_INFORMATION_STRUCTURE* GetBiosInformation(void);
SMBIOS_SYSTEM_INFORMATION_STRUCTURE* GetSystemInformation(void);
SMBIOS_BASE_BOARD_INFORMATION_STRUCTURE* GetBaseBoardInformation(void);
char* SmbiosGetString(SMBIOS_STRUCTURE_HEADER* Header, UCHAR StringOffset);


extern SMBIOS_ENTRY_POINT* SmbiosTable;