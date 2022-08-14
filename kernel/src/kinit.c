#include <kinit.h>
#include <interrupt_manager/SOD.h>
#include <krnltypes.h>
#include <sys/sys.h>
#include <Management/runtimesymbols.h>
#include <stdlib.h>
void FirmwareControlRelease(){
    // memory map code

}

void KernelHeapInitialize(){
	// Types (BIOS & UEFI)
	UINT16 LoaderData = EfiLoaderData;
	UINT16 LoaderCode = EfiLoaderCode;
	UINT16 ConventionalMemory = EfiConventionalMemory;
	if(!InitData.uefi) {
		LoaderData = -1; // BIOS May set 0xFF (who knows ??)
		LoaderCode = -1;
		ConventionalMemory = 1; // BIOS 'SMAP' Conventional Memory Entry
	}
	
	for(UINT64 i = 0;i<InitData.memory_map->count;i++){
		UINT16 Type = InitData.memory_map->mem_desc[i].type;
		if(Type == LoaderData ||
		Type == LoaderCode
		){
			
			PhysicalMemoryStatus.TotalMemory+=InitData.memory_map->mem_desc[i].pages_count << 12;
			PhysicalMemoryStatus.AllocatedMemory+=InitData.memory_map->mem_desc[i].pages_count << 12;
		}
		if(InitData.memory_map->mem_desc[i].type != ConventionalMemory
        ) continue;


			
		PhysicalMemoryStatus.TotalMemory+=InitData.memory_map->mem_desc[i].pages_count << 12;
		
		// Physical Start Of memory may be zero


		if (!InitData.memory_map->mem_desc[i].physical_start) {
			/*
				Some hardware, VMs like VBOX's conventionnal memory starts at Address 0
				And that will make the os thinks of a memory allocation issue
			*/
			InitData.memory_map->mem_desc[i].physical_start += 0x1000;
			InitData.memory_map->mem_desc[i].pages_count--;
			PhysicalMemoryStatus.AllocatedMemory += 0x1000;
		}
		if (!InitData.memory_map->mem_desc[i].pages_count) continue;
		PFREE_HEAP_DESCRIPTOR HeapDesc = AllocateFreeHeapDescriptor(
			kproc, NULL,
			(LPVOID)InitData.memory_map->mem_desc[i].physical_start,
			InitData.memory_map->mem_desc[i].pages_count << 12
		);

		if (!HeapDesc) SET_SOD_MEMORY_MANAGEMENT;
	}

	
}

void KernelPagingInitialize(){
	kproc->PageMap = (RFPAGEMAP)0x9000; // To be easily accessible by SMP
	SZeroMemory(kproc->PageMap);

	KeGlobalCR3 = (UINT64)kproc->PageMap;
	
	UINT16 LoaderData = EfiLoaderData;
	UINT16 LoaderCode = EfiLoaderCode;
	UINT16 ConventionalMemory = EfiConventionalMemory;
	UINT16 UnusableMemory = EfiUnusableMemory;
	if(!InitData.uefi) {
		LoaderData = -1; // BIOS May set 0xFF (who knows ??)
		LoaderCode = -1;
		UnusableMemory = -1;
		ConventionalMemory = 1; // BIOS 'SMAP' Conventional Memory Entry
	}
	{
		UINT64 Flags = PM_MAP | PM_NO_CR3_RELOAD | PM_NO_TLB_INVALIDATION;
		for (UINT64 i = 0; i < InitData.memory_map->count; i++) {
			UINT16 MemoryType = InitData.memory_map->mem_desc[i].type;
			if(MemoryType == UnusableMemory) continue;
			// Check if its a conventionnal memory, if not then the page writing
			//  should not be cached (page must be writting in memory not only in cache)
			if (MemoryType != LoaderData && MemoryType != LoaderCode &&
				MemoryType != ConventionalMemory) Flags |= PM_WRITE_THROUGH;
			
			MapPhysicalPages(kproc->PageMap, (LPVOID)InitData.memory_map->mem_desc[i].physical_start, (LPVOID)InitData.memory_map->mem_desc[i].physical_start, InitData.memory_map->mem_desc[i].pages_count, Flags);
			Flags = PM_MAP | PM_NO_CR3_RELOAD | PM_NO_TLB_INVALIDATION;
		}
	}
		// Map physical address of the frame buffer to the Kernel Space
		InitData.fb->FrameBufferBase = AllocateIoMemory(InitData.fb->FrameBufferBase, (InitData.fb->FrameBufferSize >> 12) + 1, PM_WRITE_THROUGH);
		// Map Kernel (BIOS Bootloader memory map is very simple and kernel memory region is not mapped)
		MapPhysicalPages(kproc->PageMap, (char*)SystemSpaceBase + SYSTEM_SPACE48_KERNEL, (char*)InitData.ImageBase, (InitData.ImageSize >> 12) + 1, PM_MAP);
		// MapPhysicalPages(kproc->PageMap, (char*)InitData.ImageBase, (char*)InitData.ImageBase, (InitData.ImageSize >> 12) + 1, PM_MAP);

		// Map Low Memory & 1MB Of high memory (Not declared by LEGACY BIOS Bootloader)
		MapPhysicalPages(kproc->PageMap, 0, 0, 1, PM_MAP | PM_LARGE_PAGES);

		// Map Allocated Memory by bootloader
		UINT64 DependencyOffset = 0;
		MapPhysicalPages(kproc->PageMap, (char*)SystemSpaceBase + SYSTEM_SPACE48_DEPENDENCIES + DependencyOffset, InitData.start_font, 0x100, PM_MAP);
		InitData.start_font = (void*)((char*)SystemSpaceBase + SYSTEM_SPACE48_DEPENDENCIES + DependencyOffset);
		DependencyOffset += 0x80000;

	// 	FILE_IMPORT_ENTRY* Entry = FileImportTable;
	// 	while(Entry->Type != FILE_IMPORT_ENDOFTABLE) {
	// 		if(Entry->BaseName){
	// 			Entry->LenBaseName = wstrlen(Entry->BaseName);
	// 		}
	// 		MapPhysicalPages(kproc->PageMap, (char*)SystemSpaceBase + SYSTEM_SPACE48_DEPENDENCIES + DependencyOffset, Entry->LoadedFileBuffer, (Entry->LoadedFileSize >> 12) + 1, PM_MAP);
	// 		Entry->LoadedFileBuffer = (char*)SystemSpaceBase + SYSTEM_SPACE48_DEPENDENCIES + DependencyOffset;
	// 		DependencyOffset += Entry->LoadedFileSize;
	// 		if(DependencyOffset & 0xFFF) {
	// 			DependencyOffset += 0x1000;
	// 			DependencyOffset &= ~0xFFF;
	// 		}
	// 		Entry++;
	// 	}

	// MapPhysicalPages(kproc->PageMap, InitData.PEDataDirectories, InitData.PEDataDirectories, 1, PM_MAP);
}

void __KernelRelocate() {
	PIMAGE_BASE_RELOCATION_BLOCK RelocationBlock = (PIMAGE_BASE_RELOCATION_BLOCK)((UINT64)InitData.ImageBase + InitData.PEDataDirectories->BaseRelocationTable.VirtualAddress);
	char* VirtualBuffer = InitData.ImageBase;
	char* ImageBase = (char*)SystemSpaceBase + SYSTEM_SPACE48_KERNEL;
	char* NextBlock = (char*)RelocationBlock;
	void* SectionEnd = NextBlock + InitData.PEDataDirectories->BaseRelocationTable.Size;
	while (NextBlock <= SectionEnd) {
		RelocationBlock = (PIMAGE_BASE_RELOCATION_BLOCK)NextBlock;
		UINT32 EntriesCount = (RelocationBlock->BlockSize - sizeof(*RelocationBlock)) / 2;
		NextBlock += RelocationBlock->BlockSize;
		if (!RelocationBlock->PageRva || !RelocationBlock->BlockSize) break;


		for (UINT32 i = 0; i < EntriesCount; i++) {
			if (RelocationBlock->RelocationEntries[i].Type != IMAGE_REL_BASED_DIR64) continue;
			UINT64* TargetRebase = (UINT64*)((char*)VirtualBuffer + RelocationBlock->PageRva + RelocationBlock->RelocationEntries[i].Offset);
			UINT64 BaseValue = *TargetRebase;

			UINT64 RawAddress = (UINT64)((char*)ImageBase + (BaseValue - (UINT64)VirtualBuffer));
			*TargetRebase = RawAddress;
		}
	}
	InitData.ImageBase = (char*)SystemSpaceBase + SYSTEM_SPACE48_KERNEL;
}

void InitFeatures(){
	IpcInit();
}