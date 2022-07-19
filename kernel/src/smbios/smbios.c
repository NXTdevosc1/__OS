#include <smbios/smbios.h>
#include <CPU/paging.h>
#include <kernel.h>
#include <preos_renderer.h>
#include <cstr.h>
#include <CPU/cpu.h>

char* SMBIOS_SIGNATURE = "_SM_";
char* SMBIOS_INTERMEDIATE_ANCHOR_STRING = "_DMI_";

SMBIOS_ENTRY_POINT* SmbiosTable = NULL;

UINT SmbiosGetStructureLength(SMBIOS_STRUCTURE_HEADER* Header) {
	UINT Length = Header->Length;
	char* Strings = (char*)((char*)Header + Length);
	for (;;) {
		if (!(*Strings) && !(*(Strings + 1))) { // Double 0 means end of structure
			Length += 2;
			break;
		}
		Length++;
		Strings++;
	}
	return Length;
}

char* SmbiosGetString(SMBIOS_STRUCTURE_HEADER* Header, UCHAR StringOffset) {
	if(!StringOffset) return NULL; // String Offset starts by 1
	StringOffset--;
	char* string = (char*)((char*)Header + Header->Length);
	for (char i = 0; i < StringOffset; i++) {
		while (*string) string++;

		string++;
		if (!(*string)) break; // Structure Ends
	}
	return string;
}

UCHAR SmbiosGetMajorVersion() {
	if (!SmbiosTable) return 0;
	return SmbiosTable->MajorVersion;
}

UCHAR SmbiosGetMinorVersion() {
	if (!SmbiosTable) return 0;
	return SmbiosTable->MinorVersion;
}

WORD SmbiosGetStructuresCount() {
	if (!SmbiosTable) return 0;
	return SmbiosTable->NumSmbiosStructures;
}

SMBIOS_STRUCTURE_HEADER* SmbiosGetStructure(WORD StructureIndex) {
	if (!SmbiosTable || StructureIndex >= SmbiosTable->NumSmbiosStructures) return NULL;

	char* SmbiosStructureOffset = (char*)((UINT64)SmbiosTable->StructureTableAddress);
	SMBIOS_STRUCTURE_HEADER* Header = (SMBIOS_STRUCTURE_HEADER*)(SmbiosStructureOffset);
	SmbiosStructureOffset += SmbiosGetStructureLength(Header);
	
	// if index > 0 then get next tables

	for (WORD i = 0; i < StructureIndex; i++) {
		Header = (SMBIOS_STRUCTURE_HEADER*)(SmbiosStructureOffset);
		if (Header->Type == SMBIOS_ENDOF_TABLE) break;
		SmbiosStructureOffset += SmbiosGetStructureLength(Header);
	}
	return Header;
}

SMBIOS_BIOS_INFORMATION_STRUCTURE* GetBiosInformation() {
	if (!SmbiosTable) return NULL;
	SMBIOS_STRUCTURE_HEADER* Header = NULL;
	for (WORD i = 0; i < SmbiosGetStructuresCount(); i++) {
		if ((Header = SmbiosGetStructure(i)) && Header->Type == SMBIOS_BIOS_INFORMATION) {
			return (SMBIOS_BIOS_INFORMATION_STRUCTURE*)Header;
		}
	}
	return NULL;
}

SMBIOS_SYSTEM_INFORMATION_STRUCTURE* GetSystemInformation() {
	if (!SmbiosTable) return NULL;
	SMBIOS_STRUCTURE_HEADER* Header = NULL;
	for (WORD i = 0; i < SmbiosGetStructuresCount(); i++) {
		if ((Header = SmbiosGetStructure(i)) && Header->Type == SMBIOS_SYSTEM_INFORMATION) {
			return (SMBIOS_SYSTEM_INFORMATION_STRUCTURE*)Header;
		}
	}
	return NULL;
}

SMBIOS_BASE_BOARD_INFORMATION_STRUCTURE* GetBaseBoardInformation() {
	if (!SmbiosTable) return NULL;
	SMBIOS_STRUCTURE_HEADER* Header = NULL;
	for (WORD i = 0; i < SmbiosGetStructuresCount(); i++) {
		if ((Header = SmbiosGetStructure(i)) && Header->Type == SMBIOS_BASE_BOARD_INFORMATION) {
			return (SMBIOS_BASE_BOARD_INFORMATION_STRUCTURE*)Header;
		}
	}
	return NULL;
}

void SystemManagementBiosInitialize(void* _SmBIOSHeader) {
	// Get The Best version of SMBIOS Tables
	
	MapPhysicalPages(kproc->PageMap, _SmBIOSHeader, _SmBIOSHeader, 1, PM_PRESENT | PM_NX);
	SMBIOS_ENTRY_POINT* SmbiosEntryPoint = (SMBIOS_ENTRY_POINT*)_SmBIOSHeader;
	
	if (SmbiosEntryPoint->EntryPointLength != sizeof(SMBIOS_ENTRY_POINT) ||
		!memcmp(SmbiosEntryPoint->IntermediateAnchorString, SMBIOS_INTERMEDIATE_ANCHOR_STRING, 5)) return;

	if(!SmbiosTable || SmbiosEntryPoint->MajorVersion > SmbiosTable->MajorVersion || 
	(SmbiosEntryPoint->MajorVersion == SmbiosTable->MajorVersion && SmbiosEntryPoint->MinorVersion > SmbiosTable->MinorVersion)
	){
		SmbiosTable = SmbiosEntryPoint;
		void* StructureTableAddress = (void*)(UINTPTR)SmbiosTable->StructureTableAddress;
		MapPhysicalPages(kproc->PageMap, StructureTableAddress, StructureTableAddress, ALIGN_VALUE(SmbiosTable->StructureTableLength, 0x1000) >> 12, PM_PRESENT | PM_NX);
	};
}