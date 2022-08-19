#include <MemoryManagement.h>
#include <fs/fs.h>
#include <kernel.h>

PAGE_ALLOCATION_TABLE* PageAllocationTable = NULL;

FILE PageFile = NULL;
BOOL _PageFilePresent = 0;


__declspec(align(0x1000)) MEMORY_MANAGEMENT_TABLE MemoryManagementTable = {0};

#define DECLARE_USED_MEMORY(NumBytes) {MemoryManagementTable.UsedMemory += (NumBytes); MemoryManagementTable.AvailableMemory -= (NumBytes);}
#define DECLARE_FREE_MEMORY(NumBytes) {MemoryManagementTable.UsedMemory -= (NumBytes); MemoryManagementTable.AvailableMemory += (NumBytes);}

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

// SSE Variants
extern RFMEMORY_SEGMENT _SSE_FetchMemoryCacheLine(MEMORY_BLOCK_CACHE_LINE* CacheLine);
// AVX Variants
extern RFMEMORY_SEGMENT _AVX_FetchMemoryCacheLine(MEMORY_BLOCK_CACHE_LINE* CacheLine);
// AVX512 Variants
extern RFMEMORY_SEGMENT _AVX512_FetchMemoryCacheLine(MEMORY_BLOCK_CACHE_LINE* CacheLine);

// SIMD Function List
RFMEMORY_SEGMENT (*_SIMD_FetchMemoryCacheLine)(MEMORY_BLOCK_CACHE_LINE* CacheLine) = _SSE_FetchMemoryCacheLine;
RFMEMORY_SEGMENT (*_SIMD_FetchUnusedSegmentsUncached)(MEMORY_SEGMENT_LIST_HEAD* ListHead) = NULL;
RFMEMORY_SEGMENT (*_SIMD_FetchSegmentListHead)(MEMORY_SEGMENT_LIST_HEAD* ListHead, UINT64 MinimumNumBytes, UINT Align) = NULL;

// ---------------------------



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
            MemoryManagementTable.PageAllocationTableSize+=Memory->PageCount;
            MemoryManagementTable.AvailableMemory += (Memory->PageCount << 12);
        }
    }
    if(MemoryManagementTable.AvailableMemory < 0x40000000) SOD(0, "Unsufficient Memory"); // Memory Must be >= 1GB

    UINT64 NumPages = ((sizeof(PAGE_ALLOCATION_TABLE) * MemoryManagementTable.PageAllocationTableSize) >> 12) + 1;
    // Search a place for PAGE_ALLOCATION_TABLE (Must be physically contiguous)
    MemoryManagementTable.PageAllocationTableSize -= NumPages;
    NumPages = ((sizeof(PAGE_ALLOCATION_TABLE) * MemoryManagementTable.PageAllocationTableSize) >> 12) + 1;

    Memory = InitData.MemoryMap->MemoryDescriptors;

    for(UINT64 i = 0;i<InitData.MemoryMap->Count;i++, Memory++) {
        if(Memory->Type == EfiConventionalMemory && Memory->PageCount > NumPages) {
            PageAllocationTable = (PAGE_ALLOCATION_TABLE*)Memory->PhysicalStart;
            (char*)Memory->PhysicalStart+=(NumPages << 12);
            Memory->PageCount -= NumPages;
            DECLARE_USED_MEMORY(NumPages << 12);
            break;
        }
    }
    if(!PageAllocationTable) SOD(0, "Cannot allocate contiguous Physical Memory for Resources.");
    Memory = InitData.MemoryMap->MemoryDescriptors;
    PAGE_ALLOCATION_TABLE* Page = PageAllocationTable;
    for(UINT64 x = 0;x<InitData.MemoryMap->Count;x++, Memory++) {
        if(Memory->Type == EfiConventionalMemory) {
            UINT64 PhysicalAddress = (Memory->PhysicalStart >> 12) << 12;
            for(UINT i = 0;i<Memory->PageCount;i++, Page++, PhysicalAddress+=0x1000) {
                Page->State.Raw = PhysicalAddress; // To initialize remaining variables as 0
            }
        }
    }

    // Declare all the free segments in the cache
    for(int i = 0;i<NUM_FREE_MEMORY_LEVELS;i++) {
        MemoryManagementTable.FreeMemoryLevels[i].UnallocatedSegmentCache.CachedMemorySegments[i] = 
        &MemoryManagementTable.FreeMemoryLevels[i].SegmentListHead.MemorySegments[i];

        MemoryManagementTable.FreeMemoryLevels[i].UnallocatedSegmentCache.CacheLineSize++;
    }
}

// Used after relocation
void SIMD_InitOptimizedMemoryManagement() {
    if(ExtensionLevel == EXTENSION_LEVEL_SSE) {
        _SIMD_FetchMemoryCacheLine = _SSE_FetchMemoryCacheLine;
    }
    else if(ExtensionLevel == EXTENSION_LEVEL_AVX) {
        _SIMD_FetchMemoryCacheLine = _AVX_FetchMemoryCacheLine;
    } else if((ExtensionLevel & EXTENSION_LEVEL_AVX512)) {
        _SIMD_FetchMemoryCacheLine = _AVX512_FetchMemoryCacheLine;
    }
}