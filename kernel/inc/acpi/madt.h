#pragma once

#include <acpi/acpi_defs.h>
#include <CPU/cpu.h>
#include <CPU/process.h>
#include <krnltypes.h>
enum ACPI_MADT_RECORDS{
	ACPI_MADT_RECORD_PROCESSOR_LAPIC = 0,
	ACPI_MADT_RECORD_IO_APIC = 1,
	ACPI_MADT_RECORD_IO_APIC_ISO = 2,
	ACPI_MADT_RECORD_IO_APIC_NMIS = 3,
	ACPI_MADT_RECORD_LOCAL_APIC_NMI = 4,
	ACPI_MADT_RECORD_LOCAL_APIC_AO = 5,
	ACPI_MADT_RECORD_LOCAL_X2_APIC = 9
};



enum ACPI_INTERRUPT_SOURCE_OVERRIDE_FLAGS{
	ISO_ACTIVE_HIGH = 1,
	ISO_ACTIVE_LOW = 3,
	ISO_EDGE_TRIGGERED = (1 << 2),
	ISO_LEVEL_TRIGGERED = (3 << 2)
};

#pragma pack(push, 1)

typedef struct _ACPI_MADT{
	ACPI_SDT Sdt;
	UINT32 LocalApicAddress;
	UINT32 Flags;
} ACPI_MADT;

typedef struct _ACPI_MADT_RECORD_HEADER{
	UINT8 EntryType;
	UINT8 RecordLength;
} ACPI_MADT_RECORD_HEADER;

// RECORD ENTRY TYPE 0
typedef struct _ACPI_PROCESSORLAPIC{
	ACPI_MADT_RECORD_HEADER RecordHeader;
	UINT8 ProcessorId;
	UINT8 ApicId;
	UINT32 Flags;	
} ACPI_PROCESSORLAPIC;

// RECORD ENTRY TYPE 1
typedef struct _ACPI_IOAPIC{
	ACPI_MADT_RECORD_HEADER RecordHeader;
	UINT8 IoApicId;
	UINT8 Reserved;
	UINT32 IoApicAddress;
	UINT32 GlobalSysInterruptBase;
} ACPI_IOAPIC;

// RECORD ENTRY TYPE 2, ISO = interrupt source override
typedef struct _ACPI_IOAPIC_ISO{
	ACPI_MADT_RECORD_HEADER RecordHeader;
	UINT8 BusSource;
	UINT8 IrqSource;
	UINT32 GlobalSysInterrupt;
	UINT16 Flags;
} ACPI_IOAPIC_ISO;

// RECORD ENTRY TYPE 3, NMIS = non-maskable interrupt source
typedef struct _ACPI_IOAPIC_NMIS{
	ACPI_MADT_RECORD_HEADER RecordHeader;
	UINT8 NmiSource;
	UINT8 Reserved;
	UINT16 Flags;
	UINT32 GlobalSysInterrupt;
} ACPI_IOAPIC_NMIS;

// RECORD ENTRY TYPE 4
typedef struct _ACPI_LAPIC_NMI{
	ACPI_MADT_RECORD_HEADER RecordHeader;
	UINT8 AcpiProcessorId;
	UINT16 Flags;
	UINT8 LINT; // 0 or 1
} ACPI_LAPIC_NMI;

//RECORD ENTRY TYPE 5, AO = address override
typedef struct _ACPI_LAPICAO{
	ACPI_MADT_RECORD_HEADER RecordHeader;
	UINT16 Reserved;
	UINT64 PhysLocalApicAddress;
} ACPI_LAPICAO;

//RECORD ENTRY TYPE 9,
typedef struct _ACPI_LAPICX2{
	ACPI_MADT_RECORD_HEADER RecordHeader;
	UINT16 Reserved;
	UINT32 X2LapicProcessorId;
	UINT32 Flags;
	UINT32 AcpiId;
} ACPI_LAPICX2;


#pragma pack(pop)

void AcpiReadMadt(ACPI_SDT* Sdt);
UINT32 AcpiGetNumProcessors(void);

UINT GetRedirectedIrq(UINT IrqNumber, UINT* IrqFlags);