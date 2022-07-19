#pragma once
#include <acpi/acpi_defs.h>
#include <CPU/process.h>
#include <interrupt_manager/idt.h>
BOOL AcpiSdtCalculateChecksum(ACPI_SDT* Sdt);
void AcpiInit(void* RsdpAddress);
unsigned char IsOemLogoDrawn();

UINT32 AcpiReadTimer();
void __cdecl AcpiSystemControlInterruptHandler(RFDRIVER_OBJECT DriverObject, RFINTERRUPT_INFORMATION InterruptInformation);
ACPI_FADT* AcpiGetFadt();