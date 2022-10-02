#include <loaders/pe64.h>
#include <fs/fs.h>
#include <sys/sys.h>
#include <stdlib.h>
#include <kernel.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>

#include <preos_renderer.h>
#include <cstr.h>


#define GetRvaSection(Rva, Image) Pe64GetSectionByVirtualAddress(Rva, Image)

#define CalculatePhysicalRva(base, section, addr) (base + section->PtrToRawData + (addr - section->VirtualAddress))

#define SYSTEM_PATH L"//OS/System/"


#define IMPORT_OSKRNLX64 "oskrnlx64.exe"

// USER PE64 LoadDll (void* ImageBuffer, void* ProgramVBuffer, PE_IMAGE* ProgImg, IMPORT_DIR ImportDir, void** (received : TargetVirtualAddress)(return : DllPhysicalAddress), 
// UINT64* (receive : RFPROCESS UserProcess)(return : UINT64 DllImageSize
HRESULT Pe64LoadDll(void* ImageBuffer, void* ProgramVirtualBuffer, PE_IMAGE_HDR* ProgramImage, PIMAGE_IMPORT_DIRECTORY ImportDirectory, void** DllImage, UINT64* DllImageSize) {
	char* dllname = ((char*)ProgramVirtualBuffer + ImportDirectory->NameRva);

	UINT64 DllNameLength = strlen(dllname);

	void* DllVirtualBase = NULL;
	if (DllImage) {
		DllVirtualBase = *DllImage;
	}

	BOOL isUser = FALSE;
	if (DllVirtualBase) isUser = TRUE;

	USHORT WDllName[MAX_FILE_NAME] = { 0 };
	WDllName[DllNameLength] = 0;
	// Convert to wide char string
	for (UINT64 i = 0; i < DllNameLength; i++) {
		WDllName[i] = dllname[i];
	}

	RFPROCESS UserProcess = NULL;
	if (DllImageSize) {
		UserProcess = (RFPROCESS)*DllImageSize;
	}

	UINT64 VirtualBufferLength = 0x1000;

	FILE DllFile = NULL;
	char* DllBuffer = NULL;
	char* VirtualBuffer = NULL;
	BOOL OSKRNLX64 = FALSE;
	PIMAGE_EXPORT_DIRECTORY ExportDirectory = NULL;
	PE_IMAGE_HDR* PeImage = NULL;
	SystemDebugPrint(L"Loading DLL...");
	if (DllNameLength == sizeof(IMPORT_OSKRNLX64) - 1/*strlen(oskrnlx64.exe)*/  && memcmp(dllname, IMPORT_OSKRNLX64, sizeof(IMPORT_OSKRNLX64) - 1)) {
		// return Pe64LinkKernelSymbols(ProgramVirtualBuffer, ProgramImage, ImportDirectory);

		OSKRNLX64 = TRUE;
		SystemDebugPrint(L"OSKRNLX64 = %x, KRNL_BASE : %x, DD : %x, EDT : %x", OSKRNLX64, InitData.ImageBase, InitData.PEDataDirectories, InitData.PEDataDirectories->ExportTable.VirtualAddress);
		goto LinkDLL;
	} else {
		for(unsigned int i = 0;FileImportTable[i].Type != FILE_IMPORT_ENDOFTABLE;i++){
			if(FileImportTable[i].Type == FILE_IMPORT_DLL && FileImportTable[i].LenBaseName == DllNameLength &&
			wstrcmp_nocs(FileImportTable[i].BaseName, WDllName, DllNameLength)
			){
				// Fork the dll
				DllBuffer = AllocatePool(FileImportTable[i].LoadedFileSize);
				if(!DllBuffer) SET_SOD_MEMORY_MANAGEMENT;
				memcpy(DllBuffer, FileImportTable[i].LoadedFileBuffer, FileImportTable[i].LoadedFileSize);
				
				goto LoadDLL;
			}
		}
	}
	SOD(0, "CANNOT_FILE_DLL");
	LPWSTR ResultPath = wstrcat(SYSTEM_PATH, WDllName);

	FILE_INFORMATION_STRUCT DllFileInfo = { 0 };
	DllFile = OpenFile(ResultPath, FILE_OPEN_READ, &DllFileInfo);
	if (!DllFile) return -4; // Can't find dll or is protected by another process
	
	

	if (!DllFileInfo.FileSize) return -1; // Invalid Dll
	DllBuffer = AllocatePool(DllFileInfo.FileSize);
	if (!DllBuffer) SET_SOD_MEMORY_MANAGEMENT;

	if (FAILED(ReadFile(DllFile, 0, NULL, DllBuffer))) return -4;

	LoadDLL:

	int PeHeaderOffset = GetPEhdrOffset(DllBuffer);
	if (!PeHeaderOffset) return -1;

	PeImage = (PE_IMAGE_HDR*)(DllBuffer + PeHeaderOffset);

	if (!PeCheckFileHeader(PeImage) || PeImage->SizeofOptionnalHeader != SIZEOF_OPTIONNAL_HEADER_DATA_DIRECTORIES ||
		PeImage->ThirdHeader.NumDataDirectories != OPTIONNAL_NUM_DATA_DIRECTORIES
		)
		return -1;

	if (!(PeImage->ThirdHeader.DllCharacteristics & IMAGE_DLL_DYNAMIC_BASE)) // Check whether is dynamic base supported
		return -1;


	ImportDirectory->TimeDataStamp = PeImage->TimeDateStamp; // Set the time/date of the dll on import
	


	if (PeImage->OptionnalDataDirectories.ExportTable.VirtualAddress) {
		// There is an export table and thats what all is about

		SystemDebugPrint(L"EXPORT_TABLE");
	
		PE_SECTION_TABLE* Section = (PE_SECTION_TABLE*)((char*)&PeImage->OptionnalHeader + PeImage->SizeofOptionnalHeader);

		
		{
			PE_SECTION_TABLE* s = Section;
			for (UINT16 i = 0; i < PeImage->NumSections; i++, s++) {
				if (!(s->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) || !s->VirtualSize) continue;
				// 0x1000 Align
				if (s->VirtualSize & 0xFFF) {
					s->VirtualSize += 0x1000;
					s->VirtualSize &= ~0xFFF;
				}
				VirtualBufferLength += s->VirtualSize;
			}
		}

		if (!VirtualBufferLength) return -1;
		VirtualBuffer = AllocatePoolEx(NULL, VirtualBufferLength, 0x1000, 0);
		if (!VirtualBuffer) SET_SOD_MEMORY_MANAGEMENT;
		if (!DllVirtualBase) DllVirtualBase = VirtualBuffer;
		Section = (PE_SECTION_TABLE*)((char*)&PeImage->OptionnalHeader + PeImage->SizeofOptionnalHeader);

		for (UINT16 i = 0; i < PeImage->NumSections; i++, Section++) {

			if (Section->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) {
				memcpy((void*)(VirtualBuffer + Section->VirtualAddress), (void*)((char*)DllBuffer + Section->PtrToRawData), Section->SizeofRawData);
				if (Section->VirtualSize > Section->SizeofRawData) {
					UINT64 UninitializedDataSize = Section->VirtualSize - Section->SizeofRawData;
					memset((void*)(VirtualBuffer + Section->VirtualAddress + Section->SizeofRawData), 0, UninitializedDataSize);
				}
			}
		}

		LinkDLL:

		// Importing Symbols
		if(OSKRNLX64) {
			VirtualBuffer = InitData.ImageBase;
			DllVirtualBase = InitData.ImageBase;
			ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(VirtualBuffer + InitData.PEDataDirectories->ExportTable.VirtualAddress);
		}else {
			ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(VirtualBuffer + PeImage->OptionnalDataDirectories.ExportTable.VirtualAddress);// (DllBuffer + ExportDataSection->PtrToRawData + (PeImage->OptionnalDataDirectories.ExportTable.VirtualAddress - ExportDataSection->VirtualAddress));
		}

		
		SystemDebugPrint(L"DLL Image Loaded. Linking to the target program...");

		// SystemDebugPrint(L"ETVA : %x, ET : %x, EDVT : %x", PeImage->OptionnalDataDirectories.ExportTable.VirtualAddress, ExportDirectory, ExportDataSection->VirtualAddress);
		SystemDebugPrint(L"NR : %x, NPR : %x, EATR : %x", ExportDirectory->NameRva, ExportDirectory->NamePointerRva, ExportDirectory->ExportAddressTableRva);

		UINT32* NamePtr = (UINT32*)(VirtualBuffer + ExportDirectory->NamePointerRva);
		char* DllName = (char*)(VirtualBuffer + ExportDirectory->NameRva);
		SystemDebugPrint(L"Dll Name : %s, VB : %x, ED : %x", DllName, VirtualBuffer, ExportDirectory);
		UINT64* ImportLookupTable = (UINT64*)((char*)ProgramVirtualBuffer + ImportDirectory->ImportLookupTable);
		UINT64* ImportAddressTable = (UINT64*)((char*)ProgramVirtualBuffer + ImportDirectory->ImportAddressTableRva);

		// PE_SECTION_TABLE* ExportSection = GetRvaSection(ExportDirectory->ExportAddressTableRva, PeImage);
		// if (!ExportSection) return -1;
		UINT32* ExportAddressTable = (UINT32*)(VirtualBuffer + ExportDirectory->ExportAddressTableRva);// CalculatePhysicalRva(DllBuffer, ExportSection, ExportDirectory->ExportAddressTableRva);
		UINT16* ExportOrdinalTable = (UINT16*)(VirtualBuffer + ExportDirectory->OrdinalTableRva); //(DllBuffer, ExportSection, ExportDirectory->OrdinalTableRva);

		while (*ImportLookupTable) {
			
			UINT64 LookupEntry = *ImportLookupTable;

			if (LookupEntry & IMAGE_ORDINAL_IMPORT_FLAG) {
				// Import By Ordinal
				UINT16 OrdinalNumber = IMAGE_ORDINAL_NUMBER(LookupEntry);
				UINT32 Rva = ExportAddressTable[OrdinalNumber];
				*ImportAddressTable = (UINT64)(VirtualBuffer + Rva);
			}
			else {
				// Import By name
				UINT32 HintNameRva = (UINT32)IMAGE_HINT_NAME_RVA(LookupEntry);
				PIMAGE_HINT_NAME_TABLE entry = (PIMAGE_HINT_NAME_TABLE)((char*)ProgramVirtualBuffer + HintNameRva);
				UINT64 ImportNameLen = strlen(entry->Name);
				
				UINT32* n = NamePtr;
				BOOL SymbolFound = FALSE;
				SystemDebugPrint(L"Searching Symbol : %s", entry->Name);
				for (UINT32 i = 0; i < ExportDirectory->NumNamePointers; i++, n++) {
					char* name = (char*)(VirtualBuffer + *n);

					if (strlen(name) == ImportNameLen &&
						memcmp(name, entry->Name, ImportNameLen)
						) {
						// Symbol Found
						UINT16 Ordinal = ExportOrdinalTable[i];
						UINT32 Rva = ExportAddressTable[Ordinal];
						UINT64 base = (UINT64)((UINT64)DllVirtualBase + Rva);
						*ImportAddressTable = base;

						SymbolFound = TRUE;
						break;
					}
				}
				if (!SymbolFound) {
					SystemDebugPrint(L"Symbol : %s Not FOUND!", entry->Name);
					while(1);
				}
				//return -2;
			}

			ImportLookupTable++;
			ImportAddressTable++; // first is for address, second one is address of symbol name
		}
		if(!OSKRNLX64) {
			if (PeImage->OptionnalDataDirectories.BaseRelocationTable.VirtualAddress) {
				if (FAILED(Pe64RelocateImage(PeImage, DllBuffer, VirtualBuffer, DllVirtualBase))) return -1;
			}

			if (PeImage->OptionnalDataDirectories.ImportAddressTable.VirtualAddress) {
				if (FAILED(Pe64ResolveImports(VirtualBuffer, DllBuffer, PeImage, UserProcess))) return -1;
			}
		}
		if (DllImage) {
			*DllImage = VirtualBuffer;
		}
		if (DllImageSize) {
			*DllImageSize = VirtualBufferLength;
		}
	}
	// if(DllFile) {
	// 	CloseFile(DllFile);
	// }
	if(OSKRNLX64) {
		SystemDebugPrint(L"OSKRNLX64 Loaded !");
	} else {
		RemoteFreePool(kproc, DllBuffer);
	}

	

	return 0;
}

HRESULT Pe64LinkKernelSymbols(void* ProgramVirtualBuffer, PE_IMAGE_HDR* ProgramImage, PIMAGE_IMPORT_DIRECTORY ImportDirectory) {
	
	UINT64* ImportLookupTable = (UINT64*)((char*)ProgramVirtualBuffer + ImportDirectory->ImportLookupTable);
	UINT64* ImportAddressTable = (UINT64*)((char*)ProgramVirtualBuffer + ImportDirectory->ImportAddressTableRva);

	while (*ImportLookupTable) {

		UINT64 LookupEntry = *ImportLookupTable;

		if (LookupEntry & IMAGE_ORDINAL_IMPORT_FLAG) {
			// Kernel symbols are not imported by ordinal
			return -1;
		}
		else {
			// Import By name
			UINT32 HintNameRva = IMAGE_HINT_NAME_RVA(LookupEntry);
			PIMAGE_HINT_NAME_TABLE entry = (PIMAGE_HINT_NAME_TABLE)((char*)ProgramVirtualBuffer + HintNameRva);
			void* Symbol = GetRuntimeSymbol(entry->Name);
			if (!Symbol) return -2;
			*ImportAddressTable = (UINT64)Symbol;
		}

		ImportLookupTable++;
		ImportAddressTable++; // first is for address, second one is address of symbol name
	}
	return 0;
}