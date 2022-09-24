#include <MemoryManagement.h>
#include <fs/fs.h>
#include <kernel.h>
#include <CPU/cpu.h>

FILE PageFile = NULL;
BOOL _PageFilePresent = 0;


__declspec(align(0x1000)) MEMORY_MANAGEMENT_TABLE MemoryManagementTable = {0};

#define DECLARE_USED_MEMORY(NumBytes) MemoryManagementTable.UsedPhysicalMemory += NumBytes;
#define DECLARE_FREE_MEMORY(NumBytes) MemoryManagementTable.UsedPhysicalMemory -= NumBytes;

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
                Memory->Type = -1;
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
            MemoryManagementTable.PhysicalMemory += (Memory->PageCount << 12);
        }
    }
    if(MemoryManagementTable.PhysicalMemory < 0x40000000) SOD(0, "Unsufficient Memory"); // Memory Must be >= 1GB
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
            (char*)Memory->PhysicalStart+=(NumPages << 12);
            Memory->PageCount -= NumPages;
            DECLARE_USED_MEMORY(NumPages << 12);
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
    memset(MemoryManagementTable.PageBitmap, 0xFF, MemoryManagementTable.NumBytesPageBitmap);
    // // Declare all the free segments in the cache
    // for(int i = 0;i<NUM_FREE_MEMORY_LEVELS;i++) {
    //     MemoryManagementTable.FreeMemoryLevels[i].UnallocatedSegmentCache.CachedMemorySegments[i] = 
    //     &MemoryManagementTable.FreeMemoryLevels[i].SegmentListHead.MemorySegments[i];

    //     MemoryManagementTable.FreeMemoryLevels[i].UnallocatedSegmentCache.CacheLineSize++;
    // }
}

// Used after relocation
void SIMD_InitOptimizedMemoryManagement() {
    // Relocate variables
    MemoryManagementTable.PageArray = AllocateIoMemory(MemoryManagementTable.PageArray, ALIGN_VALUE(MemoryManagementTable.PageArraySize, 0x1000) >> 12, 0);
    MemoryManagementTable.PageBitmap = AllocateIoMemory(MemoryManagementTable.PageBitmap, ALIGN_VALUE(MemoryManagementTable.NumBytesPageBitmap, 0x1000) >> 12, 0);

    // if(ExtensionLevel == EXTENSION_LEVEL_SSE) {

    // }
    // else if(ExtensionLevel == EXTENSION_LEVEL_AVX) {
    //     _SIMD_FetchMemoryCacheLine = _AVX_FetchMemoryCacheLine;
    // } else if((ExtensionLevel & EXTENSION_LEVEL_AVX512)) {
    //     _SIMD_FetchMemoryCacheLine = _AVX512_FetchMemoryCacheLine;
    // }
}

RFMEMORY_SEGMENT MemMgr_CreateInitialHeap(void* HeapAddress, UINT64 HeapLength) {
    return NULL;
}

RFMEMORY_SEGMENT MemMgr_FreePool(RFMEMORY_REGION_TABLE MemoryRegion, RFMEMORY_SEGMENT_LIST_HEAD ListHead, RFMEMORY_SEGMENT MemorySegment) {
    return NULL;
}