#pragma once
#include <krnltypes.h>

#pragma pack(push, 1)

typedef struct _ACPI_RSDP_DESCRIPTOR{
	UINT8 Signature[8];
	UINT8 Checksum;
	UINT8 OEMId[6];
	UINT8 Revision;
	UINT32 RsdtAddress;
} ACPI_RSDP_DESCRIPTOR;

typedef struct _ACPI_EXTENDED_RSDP_DESCRIPTOR {
    ACPI_RSDP_DESCRIPTOR RsdpDescriptor;
    UINT32 Length;
    UINT64 XsdtAddress;
    UINT8 ExtendedChecksum;
    UINT8 Reserved[3];
} ACPI_EXTENDED_RSDP_DESCRIPTOR;


typedef struct _ACPI_SDT{
	UINT8 Signature[4];
	UINT32 Length;
	UINT8 Revision;
	UINT8 Checksum;
	UINT8 OEMID[6];
	UINT8 OEMTableId[8];
	UINT32 OEMRevision;
	UINT32 CreatorId;
	UINT32 CreatorRevision;	
} ACPI_SDT;

typedef struct _ACPI_RSDT{
    ACPI_SDT Sdt;
	UINT32 SdtPtr[];//[(hdr.length - sizeof(hdr)) / 4];
} ACPI_RSDT;

typedef struct _ACPI_XSDT{
    ACPI_SDT Sdt;
    UINT64 SdtPtr[]; //[(hdr.length - sizeof(hdr)) / 8];
} ACPI_XSDT;


// GAS Stands for GenericAddressStructure

enum ACPI_GAS_VALUES{
	GAS_SYSTEM_MEMORY = 0,
	GAS_SYSTEM_IO = 1,
	GAS_PCI_CONFIGURATION_SPACE = 2,
	GAS_EMBEDDED_CONTROLLER = 3,
	GAS_SYSTEM_MANAGEMENT_BUS = 4,
	GAS_SYSTEM_CMOS = 5,
	GAS_PCI_DEVICE_BAR_TARGET = 6,
	GAS_INTELLIGENT_PLATFORM_MANAGEMENT_INFRASTRUCTURE = 7,
	GAS_GENERAL_PURPOSE_IO = 8,
	GAS_GENERIC_SERIAL_BUS = 9,
	GAS_PLATFORM_COMMUNICATION_CHANNEL = 0x0A,
	GAS_RESERVED = 0x0B - 0x7F,
	GAS_OEM_DEFINED = 0x80 - 0xFF
};

typedef struct _ACPI_GENERIC_ADDRESS_STRUCTURE
{
	UINT8 AddressSpace;
	UINT8 BitWidth;
	UINT8 BitOffset;
	UINT8 AccessSize;
	UINT64 Address;
} ACPI_GENERIC_ADDRESS_STRUCTURE;
typedef struct _ACPI_FADT{
    ACPI_SDT Sdt;
	UINT32 FirmwareCtrl;
    UINT32 Dsdt;
 
    // field used in ACPI 1.0; no longer in use, for compatibility only
    UINT8  Reserved;
 
    UINT8  PreferredPowerManagementProfile;
    UINT16 SCI_Interrupt;
    UINT32 SMI_CommandPort;
    UINT8  AcpiEnable;
    UINT8  AcpiDisable;
    UINT8  S4BIOS_REQ;
    UINT8  PSTATE_Control;
    UINT32 PM1aEventBlock;
    UINT32 PM1bEventBlock;
    UINT32 PM1aControlBlock;
    UINT32 PM1bControlBlock;
    UINT32 PM2ControlBlock;
    UINT32 PMTimerBlock;
    UINT32 GPE0Block;
    UINT32 GPE1Block;
    UINT8  PM1EventLength;
    UINT8  PM1ControlLength;
    UINT8  PM2ControlLength;
    UINT8  PMTimerLength;
    UINT8  GPE0Length;
    UINT8  GPE1Length;
    UINT8  GPE1Base;
    UINT8  CStateControl;
    UINT16 WorstC2Latency;
    UINT16 WorstC3Latency;
    UINT16 FlushSize;
    UINT16 FlushStride;
    UINT8  DutyOffset;
    UINT8  DutyWidth;
    UINT8  DayAlarm;
    UINT8  MonthAlarm;
    UINT8  Century;
 
    // reserved in ACPI 1.0; used since ACPI 2.0+
    UINT16 BootArchitectureFlags;
 
    UINT8  Reserved2;
    UINT32 Flags;
 
    // 12 byte structure; see below for details
    ACPI_GENERIC_ADDRESS_STRUCTURE ResetReg;
 
    UINT8  ResetValue;
    UINT8  Reserved3[3];
 
    // 64bit pointers - Available on ACPI 2.0+
    UINT64                X_FirmwareControl;
    UINT64                X_Dsdt;
 
    ACPI_GENERIC_ADDRESS_STRUCTURE X_PM1aEventBlock;
    ACPI_GENERIC_ADDRESS_STRUCTURE X_PM1bEventBlock;
    ACPI_GENERIC_ADDRESS_STRUCTURE X_PM1aControlBlock;
    ACPI_GENERIC_ADDRESS_STRUCTURE X_PM1bControlBlock;
    ACPI_GENERIC_ADDRESS_STRUCTURE X_PM2ControlBlock;
    ACPI_GENERIC_ADDRESS_STRUCTURE X_PMTimerBlock;
    ACPI_GENERIC_ADDRESS_STRUCTURE X_GPE0Block;
    ACPI_GENERIC_ADDRESS_STRUCTURE X_GPE1Block;
} ACPI_FADT;

#pragma pack(pop)