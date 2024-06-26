#include <acpi/acpi.h>
#include <preos_renderer.h>
#include <cstr.h>
#include <interrupt_manager/SOD.h>
#include <IO/utility.h>
#include <MemoryManagement.h>
#include <acpi/madt.h>
#include <IO/pcie.h>
#include <acpi/bgrt.h>
#include <Management/runtimesymbols.h>
#include <Management/debug.h>
#include <acpi/aml.h>
#include <acpi/hpet.h>

#define ACPI_EXTENDED_RSDP_DESCRIPTOR_SIZE 16

char* XSDT_SIGNATURE = "XSDT";
char* RSDT_SIGNATURE = "RSDT";

static const char* ACPI_SIG[] = {
	"APIC","BERT","CPEP","DSDT",
	"ECDT","EINJ","ERST","FACP",
	"FACS","HEST","MSCT","MPST",
	"OEMx","PMTT","PSDT","RASF",
	"RSDT","SBST","SLIT","SRAT",
	"SSDT","XSDT","MCFG"
};

UINT8 OemLogoDrawn = FALSE;


uint8_t sig_cmp(char* sig1, char* sig2){
	for(uint8_t i = 0;i<4;i++){
		if(sig1[i] != sig2[i]) return 0;
	}
	return 1;
}

uint8_t is_valid_rsdp_checksum(void* addr, uint8_t length){
	UINT64 checksum = 0;
	for(UINT64 i = 0;i<length;i++){
		checksum+=((char*)addr)[i];
	}
	
	return (checksum<<56) == 0 ? 1 : 0;
}

BOOL AcpiSdtCalculateChecksum(ACPI_SDT* Sdt){
	unsigned char checksum = 0;
	for (uint32_t i = 0; i < Sdt->Length; i++) {
		checksum+=((unsigned char*)Sdt)[i];
	}

	return (checksum == 0);
}
ACPI_FADT* RequiredFadtTable = NULL;

// SCI_Handler
void __cdecl AcpiSystemControlInterruptHandler(RFDRIVER_OBJECT DriverObject, RFINTERRUPT_INFORMATION InterruptInformation){
	SystemDebugPrint(L"ACPI SCI_Interrupt");
}

void AcpiInit(void* RsdpAddress){
	MapPhysicalPages(kproc->PageMap, RsdpAddress, RsdpAddress, 1, PM_MAP);
	#ifdef ___KERNEL_DEBUG___
		DebugWrite("ACPI : MAPPING RSDP");
	#endif

	MapPhysicalPages(kproc->PageMap,
	RsdpAddress, RsdpAddress, 1, PM_PRESENT | PM_NX
	);

	CreateRuntimeSymbol("$_AcpiRSDP", RsdpAddress);
	CreateRuntimeSymbol("$_AcpiAccessType", (LPVOID)2); // 1 = RSDT, 2 = XSDT (Access By 64 Bit Addresses)

	
	ACPI_EXTENDED_RSDP_DESCRIPTOR* Rsdp = (ACPI_EXTENDED_RSDP_DESCRIPTOR*)RsdpAddress;
		
	
		
		#ifdef ___KERNEL_DEBUG___
		DebugWrite("ACPI : CHECKING RSDP/XSDP Checksums");
	#endif

		UINT8 RsdpRevision = Rsdp->RsdpDescriptor.Revision;
		SystemDebugPrint(L"RSDP_REVISION : %d", RsdpRevision);
	if(RsdpRevision == 2) {
		if(!is_valid_rsdp_checksum(RsdpAddress,sizeof(ACPI_RSDP_DESCRIPTOR))
		|| !is_valid_rsdp_checksum((void*)(&Rsdp->RsdpDescriptor + 1), (UINT8)(Rsdp->Length-sizeof(ACPI_RSDP_DESCRIPTOR))))
		{
			SET_SOD_INVALID_ACPI_TABLE;
		}
	} else {
		if(!is_valid_rsdp_checksum(RsdpAddress,sizeof(ACPI_RSDP_DESCRIPTOR))) {
			SET_SOD_INVALID_ACPI_TABLE;
		}
	}
		
		ACPI_XSDT* Xsdt = (ACPI_XSDT*)(Rsdp->XsdtAddress);
		ACPI_RSDT* Rsdt = (ACPI_RSDT*)(UINT64)Rsdp->RsdpDescriptor.RsdtAddress;



		UINT64 SdtCount = 0; // Divided by 8



		if(RsdpRevision == 2) {
			// Map to get length
			MapPhysicalPages(kproc->PageMap,
			Xsdt, Xsdt, 1 , PM_PRESENT | PM_NX
			);
			MapPhysicalPages(kproc->PageMap,
			Xsdt, Xsdt, ALIGN_VALUE(Xsdt->Sdt.Length, 0x1000) >> 12 , PM_PRESENT | PM_NX
			);

			SdtCount = (Xsdt->Sdt.Length - sizeof(Xsdt->Sdt)) >> 3;
			if (!memcmp(Xsdt->Sdt.Signature, XSDT_SIGNATURE, 4)) SET_SOD_INVALID_ACPI_TABLE;
		}else {
			MapPhysicalPages(kproc->PageMap,
			Rsdt, Rsdt, 1 , PM_PRESENT | PM_NX
			);
			MapPhysicalPages(kproc->PageMap,
			Rsdt, Rsdt, ALIGN_VALUE(Rsdt->Sdt.Length, 0x1000) >> 12 , PM_PRESENT | PM_NX
			);
			SdtCount = (Rsdt->Sdt.Length - sizeof(Rsdt->Sdt)) >> 2; // Divided by 4

			if (!memcmp(Rsdt->Sdt.Signature, RSDT_SIGNATURE, 4)) SET_SOD_INVALID_ACPI_TABLE;

		}

		ACPI_SDT* AcpiSdtHeader = NULL;
		
		

		for(UINT64 i = 0;i< SdtCount;i++){	
			
			#ifdef ___KERNEL_DEBUG___
			DebugWrite("MAPPING SDT");
			DebugWrite(to_hstring64(Xsdt->SdtPtr[i]));
			DebugWrite(to_stringu64(i));
			#endif

			if(RsdpRevision == 2) {
				MapPhysicalPages(
					kproc->PageMap, (void*)Xsdt->SdtPtr[i], (void*)Xsdt->SdtPtr[i], 1, PM_PRESENT | PM_NX
				);
				AcpiSdtHeader = (ACPI_SDT*)(Xsdt->SdtPtr[i]);

			} else {
				MapPhysicalPages(
					kproc->PageMap, (void*)(UINT64)Rsdt->SdtPtr[i], (void*)(UINT64)Rsdt->SdtPtr[i], 1, PM_PRESENT | PM_NX
				);

				AcpiSdtHeader = (ACPI_SDT*)((UINT64)Rsdt->SdtPtr[i]);

			}

			#ifdef ___KERNEL_DEBUG___
		DebugWrite("EXTENDED SDT MAPPING");
	#endif
			
				MapPhysicalPages(kproc->PageMap,
				AcpiSdtHeader, AcpiSdtHeader, ALIGN_VALUE(AcpiSdtHeader->Length, 0x1000) >> 12 , PM_PRESENT | PM_NX
				);
			
			
			if (!AcpiSdtCalculateChecksum(AcpiSdtHeader)) SET_SOD_INVALID_ACPI_TABLE;

			SystemDebugPrint(L"ACPI SDT : %c%c%c%c", AcpiSdtHeader->Signature[0], AcpiSdtHeader->Signature[1], AcpiSdtHeader->Signature[2], AcpiSdtHeader->Signature[3]);
			#ifdef ___KERNEL_DEBUG___
		DebugWrite("SDT IS VALID (CHECKSUM SUCCESSFULLY CHECKED)");
	#endif
			if (memcmp(AcpiSdtHeader->Signature, "FACP", 4)) {
				RequiredFadtTable = (ACPI_FADT*)(AcpiSdtHeader);
				// Take ACPI Control
				#ifdef ___KERNEL_DEBUG___
					DebugWrite("Enabling ACPI... (Found FACP)");
				#endif
				AcpiFadt(RsdpRevision, RequiredFadtTable);
			}
			else if (memcmp(AcpiSdtHeader->Signature, "APIC", 4))
			{
				#ifdef ___KERNEL_DEBUG___
					DebugWrite("Parsing MADT Configuration (Found APIC)");
				#endif
				AcpiReadMadt(AcpiSdtHeader);
			}
			else if (memcmp(AcpiSdtHeader->Signature, "MCFG", 4)) {
				#ifdef ___KERNEL_DEBUG___
					DebugWrite("Enumerating PCIE (Found MCFG)");
				#endif
				PciExpressInit(AcpiSdtHeader);
			}
			else if (memcmp(AcpiSdtHeader->Signature, "BGRT", 4)) {
				#ifdef ___KERNEL_DEBUG___
					DebugWrite("Checking OEM Logo (Found BGRT)");
				#endif
				AcpiDrawOEMLogo(AcpiSdtHeader);
				OemLogoDrawn = TRUE;
			}else if(memcmp(AcpiSdtHeader->Signature, "HPET", 4)){
				HpetInitialize((ACPI_HPET*)AcpiSdtHeader);
			}else if(memcmp(AcpiSdtHeader->Signature, "SSDT", 4)) {
				// SystemDebugPrint(L"SSDT Found.");
				// AcpiReadDsdt((RFACPI_DSDT)AcpiSdtHeader);
			}
		}
	if(!RequiredFadtTable){
		SET_SOD_INVALID_ACPI_TABLE;
	}

}

unsigned char IsOemLogoDrawn(){
	return OemLogoDrawn;
}

UINT32 AcpiReadTimer(){
	if(RequiredFadtTable->X_PMTimerBlock.Address){
		// if its supported
		if(RequiredFadtTable->X_PMTimerBlock.AddressSpace == 0){
			// MMIO
			return *(DWORD*)RequiredFadtTable->X_PMTimerBlock.Address;
		}else if(RequiredFadtTable->X_PMTimerBlock.AddressSpace == 1){
			// IO
			return InPort((UINT16)RequiredFadtTable->X_PMTimerBlock.Address);
		}else return 0;
	}
	return 0;
}

ACPI_FADT* AcpiGetFadt(){
	return RequiredFadtTable;
}