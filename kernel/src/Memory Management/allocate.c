#include <MemoryManagement.h>
#include <xmmintrin.h>
#include <CPU/cpu.h>

RFMEMORY_SEGMENT AllocateMemorySegment(MEMORY_SEGMENT_LIST_HEAD* ListHead, MEMORY_BLOCK_CACHE_LINE* CacheLine) {
    if(CacheLine->CacheLineSize) {
        return _SIMD_AllocateMemorySegmentFromCache(CacheLine);
    } else {

    }
    return NULL;
}