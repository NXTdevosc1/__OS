#include <loaders/pe64.h>
#include <stddef.h>
#include <preos_renderer.h>
#include <cstr.h>
#include <stdlib.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <CPU/process.h>
#define GetRawAddr(ImgBuffer, VirtualAddr, Section) (ImgBuffer + Section->PtrToRawData + (VirtualAddr - Section->VirtualAddress))

#define HDRCMP(x, y) memcmp(x, y, 8)


KERNELSTATUS Pe64LoadNativeApplication(void* ImageBuffer, RFDRIVER_OBJECT DriverObject){
	if(!ImageBuffer || !isDriver(DriverObject)) return KERNEL_SERR_INVALID_PARAMETER;
	int PeOff = 0;
	if((PeOff = GetPEhdrOffset(ImageBuffer)) == -1) return KERNEL_SERR;
	PE_IMAGE_HDR* PeImage = (PE_IMAGE_HDR*)((UINTPTR)ImageBuffer + PeOff);

	if (PeImage->SizeofOptionnalHeader != SIZEOF_OPTIONNAL_HEADER_DATA_DIRECTORIES ||
	PeImage->ThirdHeader.NumDataDirectories != OPTIONNAL_NUM_DATA_DIRECTORIES ||
	!(PeImage->ThirdHeader.DllCharacteristics & IMAGE_DLL_DYNAMIC_BASE) // Security check, the loaded driver must be relocatable and support address layout randomization
	) return KERNEL_SERR; // Image can't be loaded as a driver


	PE_SECTION_TABLE* Section = (PE_SECTION_TABLE*)((char*)&PeImage->OptionnalHeader + PeImage->SizeofOptionnalHeader);
	
	PIMAGE_DATA_DIRECTORY DataDirectory = (PIMAGE_DATA_DIRECTORY)((char*)&PeImage->OptionnalHeader + sizeof(PE_OPTIONAL_HEADER) + sizeof(PE_WINDOWS_SPECIFIC_FIELDS));

	

	UINT64 VirtualBufferLength = 0x1000;
	{
		PE_SECTION_TABLE* s = Section;
		for (UINT16 i = 0; i < PeImage->NumSections; i++, s++) {
			if (!(s->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) || !s->VirtualSize) continue;
			// 0x1000 Align
			if (s->VirtualSize % 0x1000) s->VirtualSize += 0x1000 - (s->VirtualSize % 0x1000);
			VirtualBufferLength += s->VirtualSize;
		}
	}
	if (!VirtualBufferLength) return -1;
	char* VirtualBuffer = kmalloc(VirtualBufferLength);
	if (!VirtualBuffer) SET_SOD_MEMORY_MANAGEMENT;

	Section = (PE_SECTION_TABLE*)((char*)&PeImage->OptionnalHeader + PeImage->SizeofOptionnalHeader);
	for (UINT16 i = 0; i < PeImage->NumSections; i++, Section++) {

		if (Section->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) {
			memcpy((void*)(VirtualBuffer + Section->VirtualAddress), (UINT64*)((char*)ImageBuffer + Section->PtrToRawData), Section->SizeofRawData);
			if (Section->VirtualSize > Section->SizeofRawData) {
				UINT64 UninitializedDataSize = Section->VirtualSize - Section->SizeofRawData;
				memset((void*)(VirtualBuffer + Section->VirtualAddress + Section->SizeofRawData), 0, UninitializedDataSize);
			}
		}
	}

	if (PeImage->OptionnalDataDirectories.BaseRelocationTable.VirtualAddress) {
		if (FAILED(Pe64RelocateImage(PeImage, ImageBuffer, VirtualBuffer, NULL))) {
			return -1;
		}
	}

	if (PeImage->OptionnalDataDirectories.ImportTable.VirtualAddress) {
		if (FAILED(Pe64ResolveImports(VirtualBuffer, ImageBuffer, PeImage, NULL))) {
			return -1;
		}
	}

	LPVOID EntryPoint = (VirtualBuffer + PeImage->OptionnalHeader.EntryPointAddr);

	DriverObject->DriverStartup = (DRIVER_STARTUP_PROCEDURE)EntryPoint;
	DriverObject->StackSize = PeImage->ThirdHeader.StackReserve;
	free(ImageBuffer, kproc);
	return KERNEL_SOK;
}

HRESULT Pe64ResolveImports(void* VirtualBuffer, void* ImageBuffer, PE_IMAGE_HDR* PeImage, RFPROCESS UserProcess) {
	IMAGE_DATA_DIRECTORY* ImportTable = &PeImage->OptionnalDataDirectories.ImportTable;

	PIMAGE_IMPORT_DIRECTORY ImportDirectory = (PIMAGE_IMPORT_DIRECTORY)((char*)VirtualBuffer + ImportTable->VirtualAddress);
	while(ImportDirectory->NameRva){
		// Before loading dll image is set with the target virtual address of the dll
		void* DllImage = NULL;
		if (UserProcess) DllImage = (void*)((UINT64)UserProcess->DllBase + USER_IMAGE_DLL_BASE);
		if ((UINT64)DllImage > USER_IMAGE_LIMIT) return -2; // OUT OF BOUNDS
		void** PDllImage = NULL;
		void* __MapDllBase = DllImage;
		if (DllImage) PDllImage = &DllImage;
		
		UINT64 DllImageSize = (UINT64)UserProcess; // send process handle
		UINT64* PDLLImageSize = NULL;
		if (DllImageSize) PDLLImageSize = &DllImageSize;
		if (Pe64LoadDll(ImageBuffer, VirtualBuffer, PeImage, ImportDirectory, PDllImage, PDLLImageSize)) {
			if(!UserProcess){
				char* emsg = "Failed to Load the Following DLL For Driver : ";
				char* msg = strcat(emsg, (char*)((char*)VirtualBuffer + ImportDirectory->NameRva));
				SOD(SOD_DRIVER_ERROR, msg);
			}
			else {
				// Terminate user process
				return -1;
			}
		}
		if (UserProcess) {
			UserProcess->DllBase = (void*)((UINT64)UserProcess->DllBase + DllImageSize);
			// DllImage will be set after loading the dll
			if (!OpenHandle(UserProcess->Handles, NULL, 0, HANDLE_DLL, DllImage, NULL)) SET_SOD_PROCESS_MANAGEMENT;
			MapPhysicalPages(UserProcess->PageMap, __MapDllBase, DllImage, DllImageSize >> 12, PM_UMAP);
		}
		ImportDirectory++;
	}
	return KERNEL_SOK;
}

PE_SECTION_TABLE* Pe64GetSectionByVirtualAddress(UINT32 VirtualAddress, PE_IMAGE_HDR* PeImage) {
	UINT32 NewVirtAddr = (VirtualAddress >> 12) << 12; // Get the aligned address
	PE_SECTION_TABLE* Section = (PE_SECTION_TABLE*)((char*)&PeImage->OptionnalHeader + PeImage->SizeofOptionnalHeader);
	for (UINT16 i = 0; i < PeImage->NumSections; i++, Section++) {
		UINT32 MaxAddr = Section->VirtualAddress + Section->VirtualSize;
		if (Section->VirtualAddress <= NewVirtAddr && MaxAddr > NewVirtAddr) {
			return Section;
		}
	}
	return NULL;
}

HRESULT Pe64RelocateImage(PE_IMAGE_HDR* PeImage, void* ImageBuffer, void* VirtualBuffer, void* ImageBase) {
	PIMAGE_BASE_RELOCATION_BLOCK RelocationBlock = (PIMAGE_BASE_RELOCATION_BLOCK)((UINT64)VirtualBuffer + PeImage->OptionnalDataDirectories.BaseRelocationTable.VirtualAddress);
	if (!ImageBase) ImageBase = VirtualBuffer;
	char* NextBlock = (char*)RelocationBlock;
	void* SectionEnd = NextBlock + PeImage->OptionnalDataDirectories.BaseRelocationTable.Size;
	while (NextBlock <= SectionEnd) {
		RelocationBlock = (PIMAGE_BASE_RELOCATION_BLOCK)NextBlock;
		UINT32 EntriesCount = (RelocationBlock->BlockSize - sizeof(*RelocationBlock)) / 2;
		NextBlock += RelocationBlock->BlockSize;
		if (!RelocationBlock->PageRva || !RelocationBlock->BlockSize) break;


		for (UINT32 i = 0; i < EntriesCount; i++) {
			if (RelocationBlock->RelocationEntries[i].Type != IMAGE_REL_BASED_DIR64) continue;
			UINT64* TargetRebase = (UINT64*)((char*)VirtualBuffer + RelocationBlock->PageRva + RelocationBlock->RelocationEntries[i].Offset);
			UINT64 BaseValue = *TargetRebase;

			UINT64 RawAddress = (UINT64)((char*)ImageBase + (BaseValue - PeImage->ThirdHeader.ImageBase));
			*TargetRebase = RawAddress;
		}
	}
	return KERNEL_SOK;
}