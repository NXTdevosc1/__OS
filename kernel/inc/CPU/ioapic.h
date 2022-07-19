#pragma once
#include <krnltypes.h>
#include <acpi/madt.h>
#define IOAPIC_REGSEL 0 // IOAPIC Register Select
#define IOAPIC_WIN 0x10 // IOAPIC Window (Data Register

// IOAPIC ADDRESS OFFSETS

enum IOAPIC_REG{
    IOAPIC_ID = 0,
    IOAPIC_VER = 1,
    IOAPIC_ARBITRATION = 2,
    IOAPIC_REDIRECTION_TABLE_BASE = 0x10
};

// IOAPIC Redirection Table
typedef struct _IOAPIC_REDTBL {
    QWORD InterruptVector : 8;
    QWORD DeliveryMode : 3; // 0 = fixed, 1 = lowest priority, 2 = SMI, 4 = NMI, 5 = INIT, 7 = ExtINT
    QWORD DestinationMode : 1;
    QWORD DeliveryStatus : 1;
    QWORD InterruptInputPinPolarity : 1; // 0 = high active, 1 = low active
    QWORD RemoteIRR : 1;
    QWORD TriggerMode : 1; // 1 = level sensitive, 0 = edge sensitive
    QWORD InterruptMask : 1; // when set the interrupt signal is masked (disabled)
    QWORD Reserved : 39;
    QWORD DestinationField : 8;
} IOAPIC_REDTBL, *RFIOAPIC_REDTBL;


void IoApicInit(ACPI_IOAPIC* IoApic);
UINT GetNumIoApics();
ACPI_IOAPIC* QueryIoApicByPhysicalId(unsigned char IoApicId);

ACPI_IOAPIC* QueryIoApic(unsigned char VirtualIoApicId); // gets sorted ioapics

UINT32 IoApicRead(ACPI_IOAPIC* IoApic, unsigned char AddressOffset);
void IoApicWrite(ACPI_IOAPIC* IoApic, unsigned char AddressOffset, UINT32 Value);

void IoApicReadRedirectionTable(ACPI_IOAPIC* IoApic, unsigned char RedirectionTableIndex, RFIOAPIC_REDTBL RedirectionTable);
void IoApicWriteRedirectionTable(ACPI_IOAPIC* IoApic, unsigned char RedirectionTableIndex, RFIOAPIC_REDTBL RedirectionTable);

BOOL KERNELAPI KeAssignIrq(UINT IrqNumber);

void SortIoApics(); // Sorts IOAPICs by ID to create LOCAL_ID aligned IOAPIC's

unsigned char IoApicGetMaxRedirectionEntry(ACPI_IOAPIC* IoApic);