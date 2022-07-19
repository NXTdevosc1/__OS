#pragma once
#include <acpi/acpi_defs.h>
#include <CPU/process.h>
#include <stdint.h>
#pragma pack(push, 1)
typedef struct _ACPI_BGRT{
    ACPI_SDT Sdt;
    UINT16 Version;
    char Status;
    char ImageType;
    UINT64 ImageAddress;
    UINT32 OffsetX;
    UINT32 OffsetY;
} ACPI_BGRT;
#pragma pack(pop)
void AcpiDrawOEMLogo(ACPI_SDT* Sdt);