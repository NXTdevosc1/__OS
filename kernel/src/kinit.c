#include <kinit.h>
#include <interrupt_manager/SOD.h>
#include <krnltypes.h>
#include <sys/sys.h>
#include <Management/runtimesymbols.h>
void FirmwareControlRelease(){
    // memory map code

}

void KernelHeapInitialize(){
	_RT_SystemDebugPrint(L"UEFI : %x, Num Entries : %x", InitData.uefi, InitData.memory_map->count);
    
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
	MapPhysicalPages(kproc->PageMap, (LPVOID)InitData.fb->FrameBufferBase, (LPVOID)InitData.fb->FrameBufferBase, (InitData.fb->FrameBufferSize + InitData.fb->HorizontalResolution * 20) >> 12, PM_MAP | PM_WRITE_THROUGH | PM_NO_CR3_RELOAD);

	if(!InitData.uefi) {

		// Map Kernel (BIOS Bootloader memory map is very simple and kernel memory region is not mapped)
		MapPhysicalPages(kproc->PageMap, (char*)InitData.ImageBase - 0x20000, (char*)InitData.ImageBase - 0x20000, (InitData.ImageSize >> 12) + 0x100 , PM_MAP);

		// Map Low Memory & 1MB Of high memory (Not declared by LEGACY BIOS Bootloader)
		MapPhysicalPages(kproc->PageMap, 0, 0, 0x200, PM_MAP);

		// Map Allocated Memory by bootloader
		MapPhysicalPages(kproc->PageMap, InitData.start_font, InitData.start_font, 8, PM_MAP); // 0x8000 KB For PSF1 Font
	
		FILE_IMPORT_ENTRY* Entry = FileImportTable;
		while(Entry->Type != FILE_IMPORT_ENDOFTABLE) {
			MapPhysicalPages(kproc->PageMap, Entry->LoadedFileBuffer, Entry->LoadedFileBuffer, (Entry->LoadedFileSize >> 12) + 1, PM_MAP);
			Entry++;
		}
	}
}

void InitFeatures(){
	IpcInit();
}