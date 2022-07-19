#include <acpi/madt.h>
#include <MemoryManagement.h>
#include <cstr.h>
#include <interrupt_manager/SOD.h>
#include <CPU/cpu.h>
#include <preos_renderer.h>
#include <sys/sys.h>
#include <CPU/ioapic.h>
#include <IO/pit.h>
UINT32 AcpiNumProcessors = 0;
ACPI_MADT* Madt = NULL;
void AcpiReadMadt(ACPI_SDT* Sdt){
	
	Madt = (ACPI_MADT*)Sdt;
	
	ACPI_MADT_RECORD_HEADER* Record = (ACPI_MADT_RECORD_HEADER*)(Madt + 1);
	LocalApicPhysicalAddress = (void*)(UINT64)Madt->LocalApicAddress;
	
	UINT32 Offset = 0;
	UINT32 DataBufferLength = Madt->Sdt.Length - sizeof(ACPI_MADT);

	while (Offset < DataBufferLength) {
		switch (Record->EntryType) {
		case ACPI_MADT_RECORD_PROCESSOR_LAPIC:
		{

			ACPI_PROCESSORLAPIC* Lapic = (ACPI_PROCESSORLAPIC*)Record;
			if (Lapic->Flags & 3) { // if bit 0 | bit 1 are set then CPU Is Bootable
				AcpiNumProcessors++;
			}
			break;
		}
		case ACPI_MADT_RECORD_LOCAL_APIC_AO:
		{
			// Set 64 Bit Lapic Address instead of 32 Bit
			ACPI_LAPICAO* AddressOverride = (ACPI_LAPICAO*)Record;
			LocalApicPhysicalAddress = (void*)AddressOverride->PhysLocalApicAddress;
			break;
		}
		case ACPI_MADT_RECORD_IO_APIC:
		{
			ACPI_IOAPIC* IoApic = (ACPI_IOAPIC*)Record;
			IoApicInit(IoApic);
			break;
		}
		case ACPI_MADT_RECORD_IO_APIC_ISO:
		{
			// IO INTERRUPT SOURCE OVERRIDE
			// Changes in interrupts sources from PIC to IOAPIC
			// for e.g. on newer hardware (PIT maybe mapped on IRQ 2 instead of IRQ 0)
			ACPI_IOAPIC_ISO* Iso = (ACPI_IOAPIC_ISO*)Record;
			_RT_SystemDebugPrint(L"INTERRUPT SOURCE OVERRIDE From %d to %d (FLAGS : %x, BUS_SOURCE = %x)", Iso->IrqSource, Iso->GlobalSysInterrupt, Iso->Flags, Iso->BusSource);
			break;
		}
		case ACPI_MADT_RECORD_IO_APIC_NMIS:
		{
			ACPI_IOAPIC_NMIS* Nmis = (ACPI_IOAPIC_NMIS*)Record;
			// _RT_SystemDebugPrint(L"IOAPIC_NMIS : NmiSource : %d, Flags : %x, GlobalSysInt : %x", Nmis->NmiSource, Nmis->Flags, Nmis->GlobalSysInterrupt);
			break;
		}
		case ACPI_MADT_RECORD_LOCAL_APIC_NMI:
		{
			ACPI_LAPIC_NMI* LNmi = (ACPI_LAPIC_NMI*)Record;
			// _RT_SystemDebugPrint(L"LAPIC NMI : ProcessorId : %d, Flags : %x, LINT : %x", LNmi->AcpiProcessorId, LNmi->Flags, LNmi->LINT);
			break;
		}
		}
		Offset += Record->RecordLength;
		Record = (ACPI_MADT_RECORD_HEADER*)((char*)Record + Record->RecordLength);
	}
	MapPhysicalPages(kproc->PageMap, LocalApicPhysicalAddress, LocalApicPhysicalAddress, LAPIC_PAGE_COUNT, PM_MAP | PM_CACHE_DISABLE);
	EnableApic();


	SortIoApics();
	

}

UINT GetRedirectedIrq(UINT IrqNumber, UINT* IrqFlags){
	if(!Madt) return IrqNumber;

	ACPI_MADT_RECORD_HEADER* Record = (ACPI_MADT_RECORD_HEADER*)(Madt + 1);
	
	UINT32 Offset = 0;
	UINT32 DataBufferLength = Madt->Sdt.Length - sizeof(ACPI_MADT);

	while (Offset < DataBufferLength) {
		switch (Record->EntryType) {
		case ACPI_MADT_RECORD_IO_APIC_ISO:
		{
			// IO INTERRUPT SOURCE OVERRIDE
			// Changes in interrupts sources from PIC to IOAPIC
			// for e.g. on newer hardware (PIT maybe mapped on IRQ 2 instead of IRQ 0)
			ACPI_IOAPIC_ISO* Iso = (ACPI_IOAPIC_ISO*)Record;
			if(Iso->IrqSource == IrqNumber) {
				UINT Flags = Iso->Flags;
				// if bits not set then it conforms to the specification of the bus
				// in this case we will use Bus 0 (ISA) Spec until others specs are supported
				// if(!(Flags & (3 << 2))) Flags |= 1 << 2; // Set Edge sensitive
				// if(Flags & (1 << 2)){
				// 	if(!(Flags & 0b11)){
				// 		Flags |= 0b11; // ACTIVE LOW (Conforms to ISA bus)
				// 	}
				// }
				*IrqFlags = Flags;
				return Iso->GlobalSysInterrupt;
			}
			break;
		}
		}
		Offset += Record->RecordLength;
		Record = (ACPI_MADT_RECORD_HEADER*)((char*)Record + Record->RecordLength);
	}

	return 0; // The interrupt has no redirection entries
}

UINT32 AcpiGetNumProcessors() {
	return AcpiNumProcessors;
}