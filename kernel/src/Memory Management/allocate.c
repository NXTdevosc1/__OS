#include <MemoryManagement.h>
#include <CPU/cpu.h>

RFMEMORY_SEGMENT AllocateMemorySegment(RFMEMORY_REGION_TABLE MemoryRegion) {
    if(MemoryRegion->UnallocatedSegmentCache.CacheLineSize) {
        RFMEMORY_SEGMENT Segment = _SIMD_FetchMemoryCacheLine(&MemoryRegion->UnallocatedSegmentCache);
        if(Segment) return Segment;
    }

    if(MemoryRegion->TotalEntries == MemoryRegion->AllocatedEntries) {
        if(__SyncBitTestAndSet(&MemoryRegion->AllocateListControl, 0)) {
            MemoryRegion->LastListHead->NextListHead = AllocatePoolEx(kproc, sizeof(MEMORY_SEGMENT_LIST_HEAD), 0, ALLOCATE_POOL_NEW_LIST_HEAD);
            SZeroMemory(MemoryRegion->LastListHead->NextListHead);
            MemoryRegion->LastListHead = MemoryRegion->LastListHead->NextListHead;
            RFMEMORY_SEGMENT Segment = _SIMD_FetchUnusedSegmentsUncached(MemoryRegion->LastListHead);
            __BitRelease(&MemoryRegion->AllocateListControl, 0);
            return Segment;
        } else {
            // Prevent a return NULL if the free list is not yet allocated
            while(MemoryRegion->AllocateListControl, 0) __Schedule();
        }
    }
    return _SIMD_FetchUnusedSegmentsUncached(&MemoryRegion->SegmentListHead);
}


void* AllocateContiguousPages(RFPROCESS Process, UINT64 NumPages, UINT64 Flags) {
    if(!NumPages || MemoryManagementTable.AvailableMemory < (NumPages << 12)) return NULL;
    if(!Process) {
        if(Pmgrt.SystemInitialized) return NULL;
    }
    register UINT64* PageBitmap = (UINT64*)MemoryManagementTable.PageBitmap;
    UINT64* FillStart = NULL;
    UINT BitOff = 0;
    register UINT64 CurrentPages = 0;
    UINT64 LastPageAddress = 0; // Physical page number
    register PAGE* Page = MemoryManagementTable.PageArray;
    register PAGE* StartBuffer = NULL;
    void* StartAddress = NULL;
    
    for(UINT64 k = 0;k<MemoryManagementTable.NumBytesPageBitmap;k+=8, PageBitmap++) {
        register UINT64 Bitmask = *PageBitmap;
        for(UINT64 i = 0;i<0x40;i++, Page++, Bitmask >>= 1) {
            if(Bitmask & 1) {
                if(FillStart) {
                    FillStart = NULL;
                    CurrentPages = 0;
                }
            } else {
                if(!FillStart) {
                    FillStart = PageBitmap;
                    BitOff = i;
                    StartAddress = (void*)(Page->PhysicalAddress & ~(0xFFF));
                    StartBuffer = Page;
                } else {
                    if(Page->PageStruct.PhysicalPageNumber != LastPageAddress) {
                        FillStart = NULL;
                        CurrentPages = 0;
                        continue;
                    }
                }
                LastPageAddress = Page->PageStruct.PhysicalPageNumber + 1;
                CurrentPages++;
                if(NumPages == CurrentPages) {
                    // Fill it
                    register UINT64 x = BitOff;
                    register UINT64 Mask = 0;
                        UINT64* f = FillStart;
                    for(;;f++) {
                        Mask = *f;
                        for(;x<0x40;x++, StartBuffer++) {
                            if(!CurrentPages) goto Ret;
                            Mask |= (1 << x);
                            StartBuffer->PageStruct.Allocated = 1;
                            CurrentPages--;
                        }
                        *f = Mask;
                        x = 0;
                    }
                    Ret:
                    *f = Mask;
                    return StartAddress;
                }
            }
        }
    }
    return NULL;
}