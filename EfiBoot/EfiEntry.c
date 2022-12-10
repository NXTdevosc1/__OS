#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/UgaIo.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Library/FileHandleLib.h>
#define BOOT
#define ___EDK2___
#include "../../../kernel/inc/loader_defs.h"

typedef UINT64 QWORD;
typedef UINT32 DWORD;
typedef UINT16 WORD;
typedef UINT8 BYTE;


#include "../../../kernel/inc/loaders/pe.h"
#define PSF1_H_MAGIC0 0x36
#define PSF1_H_MAGIC1 0x04

#define INVALID_KERNEL Print(L"Invalid Kernel Image, Please re-install operating system.\n")
#define ALLOCATION_PROBLEM Print(L"An allocation issue has been occurred. Please upgrade your memory.\n")

#define MAX_FILE_NAME_LEN            522// (20 * (6+5+2))+1) unicode characters from EFI FAT spec (doubled for bytes)
#define LEN_FILE_INFO (sizeof(EFI_FILE_INFO) + MAX_FILE_NAME_LEN)

int memcmp(const void* x, const void* y, UINTN len) {
	for (UINTN i = 0; i < len; i++) {
		if (((const char*)x)[i] != ((const char*)y)[i]) return 0;
	}
    
	return 1;
}

int strcmp(CHAR8* x, CHAR8* y, UINTN len){
	for(UINTN i = 0;i<len;i++){
		if(x[i] != y[i]) return 0;
	}

	return 1;
}
void memset64(UINT64* dest, UINT64 Value, UINTN len) {
	for (UINTN i = 0; i < len; i++) {
		dest[i] = Value;
	}
}
void memcpy64(UINT64* dest, const UINT64* src, UINTN len) {
	for (UINTN i = 0; i < len; i++) {
		dest[i] = src[i];
	}
}


void memset32(UINT32* dest, UINT32 Value, UINTN len) {
	for (UINTN i = 0; i < len; i++, dest++) {
		*dest = Value;
	}
}
void memcpy32(UINT32* dest, UINT32* src, UINTN len) {
	for (UINTN i = 0; i < len; i++, dest++, src++) {
		*dest = *src;
	}
}
void memcpy(void* dest, const void* src, UINTN size) {
	
	if (!(size % 8)) {
		memcpy64((UINT64*)dest, (UINT64*)src, size >> 3);
	}
	else if (!(size % 4)) {
		memcpy32((UINT32*)dest, (UINT32*)src, size >> 2);
	}
	else {
		char* d = dest;
		const char* s = src;
		for (UINTN i = 0; i < size; i++, d++, s++) {
			*d = *s;
		}
	}
}
void Xmemset(void* dest, char value, UINTN size) {
	if (!(size % 8)) {
		memset64((UINT64*)dest, (UINT64)value, size >> 3);
	}
	else if (!(size % 4)) {
		memset32((UINT32*)dest, (UINT32)value, size >> 2);
	}
	else {
		char* d = (char*)dest;
		for (UINTN i = 0; i < size; i++, d++) {
			*d = value;
		}
	}
}

EFI_FILE* LoadFile(CHAR16* Path){
	EFI_FILE* Volume = NULL;
	EFI_FILE* LoadedFile = NULL;
	EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
	EFI_FILE_IO_INTERFACE* IOVolume = NULL;
    
	gBS->HandleProtocol(gImageHandle,&gEfiLoadedImageProtocolGuid,(void**)&LoadedImage);
	gBS->HandleProtocol(LoadedImage->DeviceHandle,&gEfiSimpleFileSystemProtocolGuid,(VOID*)&IOVolume);
	IOVolume->OpenVolume(IOVolume,&Volume);

	EFI_STATUS Status = Volume->Open(Volume,&LoadedFile,Path,EFI_FILE_MODE_READ,EFI_FILE_READ_ONLY);
	if(EFI_ERROR(Status)) return NULL;
	return LoadedFile;
}
char _ChFileInfo[LEN_FILE_INFO] = {0};

struct PSF1_FONT* load_psf1_font(CHAR16* Path){
 	EFI_FILE* FontFile = LoadFile(Path);
 	if(FontFile == NULL) return NULL;
	struct PSF1_FONT* Font = NULL;
    EFI_FILE_INFO* FileInfo = (EFI_FILE_INFO*)_ChFileInfo;
    UINTN FileInfoSize = LEN_FILE_INFO;
    if(EFI_ERROR(FontFile->GetInfo(FontFile, &gEfiFileInfoGuid,
    &FileInfoSize, (void*)FileInfo
    ))){
        Print(L"Failed to query file information for : PSF1_FONT\n");
        while(1);
    }
	gBS->AllocatePool(EfiLoaderData,FileInfo->FileSize, (void**)&Font);
    UINT64 FileSize = FileInfo->FileSize;
	FontFile->Read(FontFile, &FileSize, Font);

	if(Font->header.magic[0] != PSF1_H_MAGIC0
	|| Font->header.magic[1] != PSF1_H_MAGIC1
	) return NULL;
	FontFile->Close(FontFile);
	// UINTN glyph_buffer_size = Font->header.character_size * 256;
	// if(Font->header.mode == 1){// 512 GLYPH MODE
	// glyph_buffer_size = Font->header.character_size*512;
	// }
	return Font;
}

void EFIAPI UniversalGraphicsAdapaterUpdateVideo(
    FRAME_BUFFER_DESCRIPTOR* FrameBufferDescriptor,
    unsigned int x, unsigned int y, unsigned int Width, unsigned int Height
){
    EFI_UGA_DRAW_PROTOCOL* UgaProtocol = (EFI_UGA_DRAW_PROTOCOL*)FrameBufferDescriptor->Protocol;
    
        UgaProtocol->Blt(
        UgaProtocol, (EFI_UGA_PIXEL*)FrameBufferDescriptor->FrameBufferBase,
        EfiUgaBltBufferToVideo, x, y, x, y, Width, Height, 0
        );
    
}

#define GRAPHICS_INTIALIZE_SUCCESS 0
#define GRAPHICS_INTIIALIZE_FALLBACK_UGA 1

FRAME_BUFFER_DESCRIPTOR* UniversalGraphicsAdapterInitialize(UINT64* NumFrameBuffers){
	
	
    // EFI_STATUS Status = gBS->LocateProtocol(&gEfiUgaDrawProtocolGuid,NULL, (void**)&UgaProtocol);
    EFI_HANDLE* HandleBuffer = NULL;
	UINTN NumProtocolHandles = 0;
	if(EFI_ERROR(gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiUgaDrawProtocolGuid,
		NULL,
		&NumProtocolHandles,
		&HandleBuffer
	)) || !NumProtocolHandles || !HandleBuffer){
        Print(L"UNABLE TO LOCATE HANDLE BUFFER FOR U.G.A\n");
		while(1);
	}

	FRAME_BUFFER_DESCRIPTOR* FrameBuffer = NULL;
	if(EFI_ERROR(gBS->AllocatePool(EfiLoaderData, sizeof(FRAME_BUFFER_DESCRIPTOR), (void**)&FrameBuffer))){
		ALLOCATION_PROBLEM;
	}

	FRAME_BUFFER_DESCRIPTOR* ReturnedFrameBuffer = FrameBuffer;
	Xmemset((void*)FrameBuffer, 0, sizeof(FRAME_BUFFER_DESCRIPTOR));
	
	for(UINTN i = 0;i<NumProtocolHandles;i++){
		EFI_UGA_DRAW_PROTOCOL* UgaProtocol = NULL;
		EFI_STATUS Status = gBS->OpenProtocol(
		HandleBuffer[i],
		&gEfiUgaDrawProtocolGuid,
		(void**)&UgaProtocol,
		NULL,
		NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
		);


		if(EFI_ERROR(Status) || !UgaProtocol){
			Print(L"UNABLE TO OPEN U.G.A PROTOCOL(0 of %d) (%15x) ERROR : %d\n", NumProtocolHandles, HandleBuffer[0], Status);
			while(1);
		}

		

		Status = UgaProtocol->GetMode(
			UgaProtocol, &FrameBuffer->HorizontalResolution, &FrameBuffer->VerticalResolution,
			&FrameBuffer->ColorDepth, &FrameBuffer->RefreshRate
		);
		
		Print(L"U.G.A Mode:\n");
		Print(L"Current Mode : (U.G.A)\nResolution: %dx%d\nRefresh Rate : %d\nColorDepth: %d\n\n",
		FrameBuffer->HorizontalResolution, FrameBuffer->VerticalResolution,
		FrameBuffer->RefreshRate, FrameBuffer->ColorDepth
		);
		EFI_PHYSICAL_ADDRESS BackBuffer = 0;
		if(EFI_ERROR(gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, (FrameBuffer->HorizontalResolution * FrameBuffer->VerticalResolution * 4 + 0x10000) >> 12, &BackBuffer))){
			Print(L"Unable to Allocate Memory, Please upgrade your RAM.\n");
			while(1);
		}

		Xmemset((void*)BackBuffer, 0, FrameBuffer->HorizontalResolution * FrameBuffer->VerticalResolution * 4);
		FrameBuffer->FrameBufferBase = (char*)BackBuffer;
		FrameBuffer->Protocol = UgaProtocol;
		FrameBuffer->FrameBufferSize = FrameBuffer->HorizontalResolution * FrameBuffer->VerticalResolution * 4;
		if(FrameBuffer->FrameBufferSize % 0x1000) {
			FrameBuffer->FrameBufferSize+=0x1000;
			FrameBuffer->FrameBufferSize>>=12;
			FrameBuffer->FrameBufferSize<<=12;
		}
		UgaProtocol->Blt(UgaProtocol, (EFI_UGA_PIXEL*)FrameBuffer->FrameBufferBase,
		EfiBltVideoToBltBuffer, 0, 0, 0, 0, FrameBuffer->HorizontalResolution, FrameBuffer->VerticalResolution,
		0
		);

		if(EFI_ERROR(gBS->AllocatePool(EfiLoaderData, sizeof(FRAME_BUFFER_DESCRIPTOR), (void**)&FrameBuffer->Next))){
			ALLOCATION_PROBLEM;
		}
		FrameBuffer = FrameBuffer->Next;

		Xmemset((void*)FrameBuffer, 0, sizeof(FRAME_BUFFER_DESCRIPTOR));
	}
	*NumFrameBuffers = NumProtocolHandles;
	return ReturnedFrameBuffer;
}


FRAME_BUFFER_DESCRIPTOR* GraphicsOutputProtocolInitialize(UINT64* NumFrameBuffers){
	EFI_STATUS status = 0;
	
	EFI_HANDLE* HandleBuffer = NULL;
	UINTN NumProtocolHandles = 0;

	if(EFI_ERROR(gBS->LocateHandleBuffer(
		ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL,
		&NumProtocolHandles, &HandleBuffer
	)) || !NumProtocolHandles) return NULL;

	FRAME_BUFFER_DESCRIPTOR* FrameBuffer = NULL;
	if(EFI_ERROR(gBS->AllocatePool(
		EfiLoaderData, sizeof(FRAME_BUFFER_DESCRIPTOR), (void**)&FrameBuffer
	))) ALLOCATION_PROBLEM;
	FRAME_BUFFER_DESCRIPTOR* ReturnedFrameBuffer = FrameBuffer;
	Xmemset((void*)FrameBuffer, 0, sizeof(FRAME_BUFFER_DESCRIPTOR));

	for(UINT64 i = 0;i<NumProtocolHandles;i++){
		EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
		status = gBS->OpenProtocol(
			HandleBuffer[i],
			&gEfiGraphicsOutputProtocolGuid,
			(void**)&gop,
			NULL,
			NULL,
			EFI_OPEN_PROTOCOL_GET_PROTOCOL
		);
		if(EFI_ERROR(status) || !gop)
		{
			return NULL;
		}

		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* ginfo = NULL;
		UINTN info_size = 0;
		if(status == EFI_NOT_STARTED || !gop->Mode){
			
			status = gop->SetMode(gop, 0);
			if(EFI_ERROR(status) || !gop->Mode->Mode)
			{
				return NULL;
			}
		}


		
		status = gop->QueryMode(gop, gop->Mode->Mode, &info_size, &ginfo);
		
		if(status != EFI_SUCCESS || !ginfo){
			return NULL;
		}
		
		// GOP is not supported, then initialize UGA (Older Graphics for EFI not UEFI)
		if(!gop->Mode->FrameBufferBase) {
			Print(L"Firmware maybe too old for G.O.P, reverting to U.G.A Protocol\n");
			return NULL;
		}
		
		FrameBuffer->FrameBufferSize = gop->Mode->FrameBufferSize;
		FrameBuffer->HorizontalResolution = gop->Mode->Info->HorizontalResolution;
		FrameBuffer->VerticalResolution = gop->Mode->Info->VerticalResolution;
		FrameBuffer->FrameBufferBase = (char*)gop->Mode->FrameBufferBase;
		Print(L"Current Mode : %d\nResolution: %dx%d\nFrame Buffer Base:%15x\nFrame Buffer Size:%15x\nVersion:%d\n\n",
		gop->Mode->Mode, FrameBuffer->HorizontalResolution, FrameBuffer->VerticalResolution,
		(UINT64)FrameBuffer->FrameBufferBase, FrameBuffer->FrameBufferSize, gop->Mode->Info->Version
		);
		FrameBuffer->Protocol = gop;
		if(EFI_ERROR(gBS->AllocatePool(
			EfiLoaderData, sizeof(FRAME_BUFFER_DESCRIPTOR), (void**)&FrameBuffer->Next
		))) ALLOCATION_PROBLEM;

		FrameBuffer = FrameBuffer->Next;

		Xmemset((void*)FrameBuffer, 0, sizeof(FRAME_BUFFER_DESCRIPTOR));

	}
	
	*NumFrameBuffers = NumProtocolHandles;

	
	return ReturnedFrameBuffer;

}

EFI_STATUS InitializeBootGraphics(FRAME_BUFFER_DESCRIPTOR** FrameBuffer, UINT64* NumFrameBuffers){
	if((*FrameBuffer = GraphicsOutputProtocolInitialize(NumFrameBuffers))){
		return GRAPHICS_INTIALIZE_SUCCESS;
	}else if((*FrameBuffer = UniversalGraphicsAdapterInitialize(NumFrameBuffers))){
		return GRAPHICS_INTIIALIZE_FALLBACK_UGA;
	}else {
		Print(L"Failed to initialize graphics.\nA unexpected error occured.\n");
		while(1);
	}
}

int GetPEOffset(void* hdr) {
	if (!memcmp(hdr, "MZ", 2)) return 0;
	return *(int*)((char*)hdr + 0x3c);
}

int PeCheckKernelImageHeader(PE_IMAGE_HDR* hdr) {
	if (
		hdr->ThirdHeader.Subsystem != PE_SUBSYSTEM_NATIVE ||
		hdr->MachineType != PE_TARGET_MACHINE ||
		hdr->SizeofOptionnalHeader < sizeof(PE_OPTIONAL_HEADER) ||
		!hdr->OptionnalHeader.EntryPointAddr ||
		hdr->NumSections > PE_SECTION_LIMIT ||
		!(hdr->ThirdHeader.DllCharacteristics & IMAGE_DLL_DYNAMIC_BASE) ||
		!(hdr->Characteristics & PE_LARGE_ADDRESS_AWARE) ||
		!memcmp(hdr->Signature, PE_IMAGE_SIGNATURE, 4) ||
		!(hdr->OptionnalDataDirectories.BaseRelocationTable.VirtualAddress)
		) {
		return 0;
	}
	return 1;
}

BOOLEAN PeRelocateImage(PE_IMAGE_HDR* PeImage, void* ImageBuffer, void* VirtualBuffer, void* ImageBase) {
	if (!PeImage || !ImageBuffer || !VirtualBuffer) return FALSE;
	PIMAGE_BASE_RELOCATION_BLOCK RelocationBlock = (PIMAGE_BASE_RELOCATION_BLOCK)((UINT64)VirtualBuffer + PeImage->OptionnalDataDirectories.BaseRelocationTable.VirtualAddress);
	if (!RelocationBlock) return FALSE;
	if (!ImageBase) ImageBase = VirtualBuffer;
	char* NextBlock = (char*)RelocationBlock;
	char* SectionEnd = NextBlock + PeImage->OptionnalDataDirectories.BaseRelocationTable.Size;
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
	return TRUE;
}
char _ChKernelFileInfo[LEN_FILE_INFO] = {0};
char _BufferFileInfo[LEN_FILE_INFO] = {0};
EFI_STATUS EFIAPI UefiEntry (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
	gST = SystemTable;
    gBS = SystemTable->BootServices;
    gImageHandle = ImageHandle;
    SystemTable->BootServices->SetWatchdogTimer(0,0,0,NULL);
	Print(L"(U)EFI Hello World\nBoot Manager Version:1.0\n");
	Print(L"(U)EFI Version : %d.%d\n\n", SystemTable->Hdr.Revision >> 16, SystemTable->Hdr.Revision & 0xFFFF);
	EFI_FILE* kernel = LoadFile(L"OS\\System\\OSKRNLX64.exe");
    EFI_FILE_INFO* KernelFileInfo = (EFI_FILE_INFO*)_ChKernelFileInfo;
    UINTN KernelFileInfoSize = LEN_FILE_INFO;
	if(!kernel){
		Print(L"Failed to load kernel. Please re-install operating system.\n");
		while(1);
	}
     if(EFI_ERROR(kernel->GetInfo(kernel, &gEfiFileInfoGuid, &KernelFileInfoSize, (void*)KernelFileInfo))){
        Print(L"Failed to get kernel information. Please re-install operating system.\n");
		while(1);
     }
    
	FRAME_BUFFER_DESCRIPTOR* fb = NULL;
	UINT64 NumFrameBuffers = 0;
	if(InitializeBootGraphics(&fb, &NumFrameBuffers) == GRAPHICS_INTIIALIZE_FALLBACK_UGA){
		Print(L"UEFI Graphics Output Protocol Not Support, Please consider booting via LEGACY BIOS (No need to re-install OS Boot is Hybrid)");
	}

	struct PSF1_FONT* zap_light_font = load_psf1_font(L"OS\\Fonts\\zap-light16.psf");
	if(!zap_light_font)
	{
		Print(L"Failed to load resources. Please re-install operating system.\n");
		while(1);
	}
	UINTN size = KernelFileInfo->FileSize;

	void* KernelBuffer = NULL;

	if (EFI_ERROR(SystemTable->BootServices->AllocatePool(EfiLoaderData, size + 0x10000, &KernelBuffer))) {
		ALLOCATION_PROBLEM;
		while (1);
	}

	kernel->Read(kernel, &size, KernelBuffer);
	int _PeHdrOff = 0;
	if (!(_PeHdrOff = GetPEOffset(KernelBuffer))) {
		INVALID_KERNEL;
		while (1);
	}

	PE_IMAGE_HDR* PeHdr = (PE_IMAGE_HDR*)((UINTN)KernelBuffer + _PeHdrOff);


	if (!PeCheckKernelImageHeader(PeHdr)) {
		INVALID_KERNEL;
		while (1);
	}

	PE_SECTION_TABLE* Section = (PE_SECTION_TABLE*)((char*)&PeHdr->OptionnalHeader + PeHdr->SizeofOptionnalHeader);

	INITDATA* InitData = NULL;


	{
		PE_SECTION_TABLE* s = Section;
		for (UINT16 i = 0; i < PeHdr->NumSections; i++, s++) {
			if (s->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) {
				// Optimization & Protective Padding

				if (s->VirtualSize < s->SizeofRawData) s->VirtualSize = s->SizeofRawData;
			}
		}
	}
	UINT64 SizeVBuffer = 0;
	{
		PE_SECTION_TABLE* s = Section;
		for (UINT16 i = 0; i < PeHdr->NumSections; i++, s++) {
			if (s->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) {
				// Optimization & Protective Padding

				if (s->VirtualAddress + s->VirtualSize > SizeVBuffer) SizeVBuffer = s->VirtualAddress + s->VirtualSize;
			}
		}
	}
	
	if (!SizeVBuffer || SizeVBuffer > PeHdr->ThirdHeader.SizeofImage) {
		INVALID_KERNEL;
		while (1);
	}
	SizeVBuffer += 0x20000;
	EFI_PHYSICAL_ADDRESS ImageBase = 0;

	if (EFI_ERROR(SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, SizeVBuffer >> 12, &ImageBase)) 
		|| !ImageBase
		) {
		ALLOCATION_PROBLEM;
		while (1);
	}
	
	FILE_IMPORT_ENTRY* ImportFileTable = NULL;
	char* VirtualBuffer = (char*)ImageBase;
	for (UINT16 i = 0; i < PeHdr->NumSections; i++, Section++) {
		
		if (Section->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) {
			memcpy((void*)(VirtualBuffer + Section->VirtualAddress), (UINT64*)((char*)KernelBuffer + Section->PtrToRawData), Section->SizeofRawData);
			if (Section->VirtualSize > Section->SizeofRawData) {
				UINT64 UninitializedDataSize = Section->VirtualSize - Section->SizeofRawData;
				Xmemset((void*)(VirtualBuffer + Section->VirtualAddress + Section->SizeofRawData), 0, UninitializedDataSize);
			}
		}

		if (memcmp(Section->name, _INITDATA, 8)) {
			if (InitData) {
				INVALID_KERNEL;
				while (1);
			}
			InitData = (INITDATA*)(VirtualBuffer + Section->VirtualAddress);
		}
		if(memcmp(Section->name, _FIMPORT, 8)){
			ImportFileTable = (FILE_IMPORT_ENTRY*)(VirtualBuffer + Section->VirtualAddress);
		}
	}
	
	if (!InitData || !ImportFileTable) {
		INVALID_KERNEL;
		while (1);
	}
	
	if (!PeRelocateImage(PeHdr, (void*)KernelBuffer, (void*)VirtualBuffer, NULL)) {
		INVALID_KERNEL;
		while (1);
	}
	
	while(ImportFileTable->Type != FILE_IMPORT_ENDOFTABLE){
		EFI_FILE* ImportedFile = LoadFile(ImportFileTable->Path);
		UINTN SizeofInfo = LEN_FILE_INFO;
		EFI_FILE_INFO* FileInfo = (EFI_FILE_INFO*)_BufferFileInfo;
		if(!ImportedFile || EFI_ERROR(ImportedFile->GetInfo(
			ImportedFile, &gEfiFileInfoGuid, &SizeofInfo, FileInfo
		) || !FileInfo->FileSize)
		){
			Print(L"Unable to load System File : %ls", ImportFileTable->Path);
			while(1);
		}

		ImportFileTable->LoadedFileSize = FileInfo->FileSize;
		if(ImportFileTable->LoadedFileSize & 0xFFF) {
			ImportFileTable->LoadedFileSize += 0x1000;
			ImportFileTable->LoadedFileSize &= ~0xFFF;
		}
		
		if(EFI_ERROR(SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, ImportFileTable->LoadedFileSize >> 12, &ImportFileTable->LoadedFileBuffer)))
		{
			ALLOCATION_PROBLEM;
			while(1);
		}

		if(EFI_ERROR(ImportedFile->Read(ImportedFile, &ImportFileTable->LoadedFileSize, ImportFileTable->LoadedFileBuffer))){
			Print(L"Unable to load System File : %ls", ImportFileTable->Path);
			while(1);
		}
		ImportedFile->Close(ImportedFile);
		ImportFileTable++;
	}
	void (* _start)() = ((void(*)())(VirtualBuffer + PeHdr->OptionnalHeader.EntryPointAddr));

	InitData->ImageBase = (void*)ImageBase;
	InitData->ImageSize = SizeVBuffer;
	InitData->PEDataDirectories = (RFOPTIONNAL_HEADER_DATA_DIRECTORIES)&PeHdr->OptionnalDataDirectories;
	kernel->Close(kernel);


	EFI_MEMORY_DESCRIPTOR* memory_map = NULL;
	UINTN map_size = 0, map_key = 0, descriptor_size = 0;
	UINT32 descriptor_version = 0;
	{
		UINT64 SMP_ALLOCATE_AREA = 0x8000;
		if (EFI_ERROR(SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, 2, (EFI_PHYSICAL_ADDRESS*)&SMP_ALLOCATE_AREA))) {
			Print(L"Failed to allocate SMP Data. Please report the problem to our help center.");
			while (1);
		}
	}
	
	EFI_STATUS s = SystemTable->BootServices->GetMemoryMap(&map_size,memory_map,&map_key,&descriptor_size,&descriptor_version);
	
	if(s != EFI_BUFFER_TOO_SMALL){
		Print(L"A problem occurred while getting memory map from firmware \nExitting Boot Services (STAGE 1: Get Memory Map) (EXPECTED_STATUS : EFI_BUFFER_TOO_SMALL).\nPlease inform your firmware provider.\n");
		while(1);
	}
	map_size+=2*descriptor_size; // this padding must be added for map size

	SystemTable->BootServices->AllocatePool(EfiLoaderData,0x10000 + (map_size / descriptor_size) * sizeof(MEMORY_MAP),(void**)&InitData->MemoryMap);

	Xmemset(InitData->MemoryMap, 0, (map_size / descriptor_size) * sizeof(MEMORY_MAP));
	s = SystemTable->BootServices->AllocatePool(EfiLoaderData,map_size + 0x10000,(void**)&memory_map);
	if(s != EFI_SUCCESS){
        ALLOCATION_PROBLEM;
		while(1);
	}

	s = SystemTable->BootServices->GetMemoryMap(&map_size,memory_map,&map_key,&descriptor_size,&descriptor_version);

	if(s != EFI_SUCCESS || !memory_map){
		Print(L"FAILED TO GET MEMORY MAP FROM FIRMWARE. Please inform your firmware provider.\n");
		while(1);
	}
		InitData->NumConfigurationTables = SystemTable->NumberOfTableEntries;
		InitData->RuntimeServices = SystemTable->RuntimeServices;
		InitData->SystemConfigurationTables = SystemTable->ConfigurationTable;
		InitData->EfiSystemTable = SystemTable;
	
	EFI_MEMORY_DESCRIPTOR* mp_entry = NULL;
	MEMORY_DESCRIPTOR* MemoryDescriptor = InitData->MemoryMap->MemoryDescriptors;
	for(UINTN i = 0;i<(map_size/descriptor_size) - 2;i++, MemoryDescriptor++){
		mp_entry = (EFI_MEMORY_DESCRIPTOR*) ((UINTN)memory_map+(i*descriptor_size));
			
			MemoryDescriptor->Type = mp_entry->Type;
			MemoryDescriptor->PageCount = mp_entry->NumberOfPages;
			MemoryDescriptor->PhysicalStart = (unsigned long long)mp_entry->PhysicalStart;
			InitData->MemoryMap->Count++;
	}

	s = SystemTable->BootServices->ExitBootServices(ImageHandle,map_key);
	if(s != EFI_SUCCESS){
		Print(L"FAILED_TO_EXIT_BOOT_SERVICES. Please inform your firmware provider.\n");
		while(1);
	}

	
	InitData->fb = fb;
	InitData->start_font = zap_light_font;
	InitData->uefi = 1;
	InitData->NumFrameBuffers = NumFrameBuffers;
	

	
	_start();

	return EFI_SUCCESS;
}

