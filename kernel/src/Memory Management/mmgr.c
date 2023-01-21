#include <MemoryManagement.h>
#include <fs/fs.h>
#include <kernel.h>
#include <CPU/cpu.h>
#include <interrupt_manager/SOD.h>
FILE PageFile = NULL;
BOOL _PageFilePresent = 0;


__declspec(align(0x1000)) MEMORY_MANAGEMENT_TABLE MemoryManagementTable = {0};

#define DECLARE_USED_MEMORY(NumPages) MemoryManagementTable.AllocatedPages += NumPages;
#define DECLARE_FREE_MEMORY(NumPages) MemoryManagementTable.AllocatedPages -= NumPages;

// Maps BIOS Map to EFI Memory Map Types
char LegacyBiosMemoryMapToEfiMapping[8] = {
    EfiReservedMemoryType, // Unset in SMAP
    EfiConventionalMemory,
    EfiReservedMemoryType,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiUnusableMemory,
    EfiUnusableMemory, // Disabled Memory
    EfiUnusableMemory, // Persistent Memory
};




// Can be used before kernel relocation ? Yes
void InitMemoryManagementSubsystem() {
    MEMORY_DESCRIPTOR* Memory = InitData.MemoryMap->MemoryDescriptors;
    
    for(UINT64 i = 0;i<InitData.MemoryMap->Count;i++, Memory++) {
        if(!InitData.uefi) {
            if(Memory->Type > 7) {
                Memory->Type = 0xFF;
                continue;
            }
            Memory->Type = LegacyBiosMemoryMapToEfiMapping[Memory->Type];
        }
        if(Memory->Type == EfiConventionalMemory && Memory->PageCount) {
            if(!(Memory->PhysicalStart >> 12)) {
                Memory->PhysicalStart = 0x1000;
                Memory->PageCount--;
            }
            if(Memory->PhysicalStart & 0xFFF) {
                Memory->PhysicalStart += 0x1000;
                Memory->PhysicalStart &= ~0xFFF;
                Memory->PageCount--;
            }
            MemoryManagementTable.PageArraySize+=Memory->PageCount;
            MemoryManagementTable.PhysicalPages += Memory->PageCount;
        }
    }
    UINT64 NumPages = ((sizeof(PAGE) * MemoryManagementTable.PageArraySize) >> 12) + 1;
    UINT64 BitmapOffset = NumPages << 12;
    NumPages += (((MemoryManagementTable.PageArraySize + 1) >> 3) >> 12) + 10; // Bitmap
    MemoryManagementTable.PageArraySize -= NumPages;
    // Search a place for PAGE_ALLOCATION_TABLE (Must be physically contiguous)
    // NumPages = ((sizeof(PAGE) * MemoryManagementTable.PageArraySize) >> 12) + 1;

    Memory = InitData.MemoryMap->MemoryDescriptors;

    for(UINT64 i = 0;i<InitData.MemoryMap->Count;i++, Memory++) {
        if(Memory->Type == EfiConventionalMemory && Memory->PageCount >= NumPages) {
            MemoryManagementTable.PageArray = (PAGE*)Memory->PhysicalStart;
            MemoryManagementTable.PageBitmap = (char*)Memory->PhysicalStart + BitmapOffset;
            (UINT64)Memory->PhysicalStart+=(NumPages << 12);
            Memory->PageCount -= NumPages;
            DECLARE_USED_MEMORY(NumPages);
            break;
        }
    }
    if(!MemoryManagementTable.PageArray) SOD(0, "Cannot allocate contiguous Physical Memory for Resources.");
    Memory = InitData.MemoryMap->MemoryDescriptors;
    PAGE* Page = MemoryManagementTable.PageArray;
    for(UINT64 x = 0;x<InitData.MemoryMap->Count;x++, Memory++) {
        if(Memory->Type == EfiConventionalMemory) {
            UINT64 PhysicalAddress = Memory->PhysicalStart;
            for(UINT i = 0;i<Memory->PageCount;i++, Page++, PhysicalAddress+=0x1000) {
                Page->PhysicalAddress = PhysicalAddress;
            }
        }
    }
    // Zero Page Bitmap
    MemoryManagementTable.NumBytesPageBitmap = ((((MemoryManagementTable.PageArraySize + 1) >> 12) >> 3) << 12) + 0x80; // Padding 0x80
    memset((void*)MemoryManagementTable.PageBitmap, 0, MemoryManagementTable.NumBytesPageBitmap);
    // // Declare all the free segments in the cache
    // for(int i = 0;i<NUM_FREE_MEMORY_LEVELS;i++) {
    //     MemoryManagementTable.FreeMemoryLevels[i].UnallocatedSegmentCache.CachedMemorySegments[i] = 
    //     &MemoryManagementTable.FreeMemoryLevels[i].SegmentListHead.MemorySegments[i];

    //     MemoryManagementTable.FreeMemoryLevels[i].UnallocatedSegmentCache.CacheLineSize++;
    // }
    MemoryManagementTable.FreePagesStart = MemoryManagementTable.PageBitmap;
    MemoryManagementTable.FreePageArrayStart = MemoryManagementTable.PageArray;
    MemoryManagementTable.BmpEnd = MemoryManagementTable.PageBitmap + (((MemoryManagementTable.PageArraySize) >> 3) >> 12);

}
extern LPVOID __fastcall _SSE_AllocatePhysicalPage();
LPVOID (__fastcall *_SIMD_AllocatePhysicalPage) () = _SSE_AllocatePhysicalPage;

// Used after relocation
void SIMD_InitOptimizedMemoryManagement() {
	
    // Relocate variables
    MemoryManagementTable.PageArray = (PAGE*)AllocateIoMemory((LPVOID)MemoryManagementTable.PageArray, ALIGN_VALUE(MemoryManagementTable.PageArraySize, 0x1000) >> 12, 0);
    MemoryManagementTable.PageBitmap = (char*)AllocateIoMemory((LPVOID)MemoryManagementTable.PageBitmap, ALIGN_VALUE(MemoryManagementTable.NumBytesPageBitmap, 0x1000) >> 12, 0);

    MemoryManagementTable.FreePagesStart = MemoryManagementTable.PageBitmap;
    MemoryManagementTable.FreePageArrayStart = MemoryManagementTable.PageArray;
    MemoryManagementTable.BmpEnd = MemoryManagementTable.PageBitmap + ALIGN_VALUE((((MemoryManagementTable.PageArraySize + 1) >> 3) >> 12) + 0x1000, 0x1000);


    if(ExtensionLevel == EXTENSION_LEVEL_SSE) {
        _SIMD_AllocatePhysicalPage = _SSE_AllocatePhysicalPage;
    }
    else if(ExtensionLevel == EXTENSION_LEVEL_AVX) {
        _SIMD_AllocatePhysicalPage = _SSE_AllocatePhysicalPage;

        // _SIMD_FetchMemoryCacheLine = _AVX_FetchMemoryCacheLine;
    } else if((ExtensionLevel & EXTENSION_LEVEL_AVX512)) {
        // _SIMD_FetchMemoryCacheLine = _AVX512_FetchMemoryCacheLine;
    }
}

RFMEMORY_SEGMENT MemMgr_FreePool(RFMEMORY_REGION_TABLE MemoryRegion, RFMEMORY_SEGMENT_LIST_HEAD ListHead, RFMEMORY_SEGMENT MemorySegment) {
    return NULL;
}