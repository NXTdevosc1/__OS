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