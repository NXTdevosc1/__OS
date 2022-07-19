#pragma once
#include <acpi/acpi_defs.h>
#include <krnltypes.h>
#pragma pack(push, 1)
typedef struct _ACPI_HPET{
    ACPI_SDT Sdt;
    UINT8 HardwareRevisionId;
    UINT8 ComparatorCounter : 5;
    UINT8 CounterSize : 1;
    UINT8 Reserved0 : 1;
    UINT8 LegacyReplacement : 1;
    UINT16 PciVendorId;
    ACPI_GENERIC_ADDRESS_STRUCTURE Address;
    UINT8 HpetNumber;
    UINT16 MinimumTick;
    UINT8 PageProtection;
} ACPI_HPET;

#pragma pack(pop)