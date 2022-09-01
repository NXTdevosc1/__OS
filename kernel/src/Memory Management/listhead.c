#include <MemoryManagement.h>
#include <CPU/cpu.h>
RFMEMORY_SEGMENT FindFreePhysicalMemory(RFMEMORY_REGION_TABLE MemoryRegion, UINT64 NumBytes, UINT Align) {
    RFMEMORY_SEGMENT_LIST_HEAD ListHead = &MemoryRegion->SegmentListHead;
    RFMEMORY_SEGMENT_LIST_HEAD Previous = NULL;
    for(;;) {
        if(ListHead->Bitmask) {
            MEMORY_SEGMENT* MemorySegment = ListHead->MemorySegments;
            UINT64 BitMask = ListHead->Bitmask;
            for(UINT i = 0;i<MEMORY_LIST_HEAD_SIZE;i++, MemorySegment++, BitMask >>= 1) {
                if(!BitMask) break;
                if((BitMask & 1) &&
                MemorySegment->BlockSize >= NumBytes
                ) {
                    __SpinLockSyncBitTestAndSet(&MemorySegment->Flags, MEMORY_SEGMENT_SEMAPHORE);
                    if(((UINT64)MemorySegment->BlockAddress & (Align - 1)) + NumBytes > MemorySegment->BlockSize) {
                        __BitRelease(&MemorySegment->Flags, MEMORY_SEGMENT_SEMAPHORE);
                        continue;
                    }
                    NumBytes = ((UINT64)MemorySegment->BlockAddress & (Align - 1)) + NumBytes;

                    void* BlockAddress = (void*)ALIGN_VALUE((UINT64)MemorySegment->BlockAddress, Align);

                    (char*)MemorySegment->BlockAddress += NumBytes;
                    MemorySegment->BlockSize -= NumBytes;
                    if(!MemorySegment->BlockSize) {
                        ListHead->Bitmask &= ~((UINT64)1 << i);
                        MemoryRegion->AllocatedEntries--;
                        if(Previous &&
                        ListHead->Bitmask == 1 /*Where 1 is the entry specifiying the heap for this list head*/
                        ) {
                            RFMEMORY_SEGMENT_LIST_HEAD FreePoolListHead = NULL;
                            // Free and unlink the list head
                            RFMEMORY_SEGMENT FreeMemorySegment = MemMgr_FreePool(MemoryRegion, ListHead, &ListHead->MemorySegments[0]);
                            Previous->NextListHead = ListHead->NextListHead;
                            // Remove semaphore bit from our free pool
                            __BitRelease(&FreeMemorySegment->Flags, MEMORY_SEGMENT_SEMAPHORE);
                        }
                    }

                    __BitRelease(&MemorySegment->Flags, MEMORY_SEGMENT_SEMAPHORE);
                }
            }
        }
        if(!ListHead->NextListHead) return NULL;
        Previous = ListHead;
        ListHead = ListHead->NextListHead;
    }
}