#pragma once
#include <krnltypes.h>
#include <mem.h>

#define ZeroMemory(Memory, Size) memset(Memory, 0, Size)
#define SZeroMemory(Memory) memset(Memory, 0, sizeof(*Memory))

#pragma pack(push, 1)

typedef struct _PROCESS_CONTROL_BLOCK *RFPROCESS;

typedef struct _PAGE_ALLOCATION_TABLE {
    union {
        struct {
            UINT64 Used : 1;
            UINT64 Paged : 1;
            UINT64 NumHeaps : 9; // 0-256
            UINT64 Reserved : 1;
            UINT64 PhysicalAddress : 52;
        } State;
        UINT64 Raw; // Uncleared at startup
    } State;
    
    UINT64 NextPage;
} PAGE_ALLOCATION_TABLE;

extern PAGE_ALLOCATION_TABLE* PageAllocationTable;

typedef struct _MEMORY_SEGMENT MEMORY_SEGMENT, *RFMEMORY_SEGMENT;

#define MEMORY_BLOCK_CACHE_COUNT 4
#define MEMORY_BLOCK_CACHE_SIZE 0x40


typedef struct _MEMORY_BLOCK_CACHE_LINE{
    UINT CacheLineSize;
    UINT Reserved[3]; // to keep 16 Byte alignment
    RFMEMORY_SEGMENT CachedMemorySegments[MEMORY_BLOCK_CACHE_SIZE];
} MEMORY_BLOCK_CACHE_LINE;

#define MEMORY_LIST_HEAD_SIZE 0x40
#define DEFAULT_MEMORY_ALIGNMENT 0x10

typedef enum _MEMORY_SEGMENT_FLAGS {
    MEMORY_SEGMENT_PRESENT = 1,
    MEMORY_SEGMENT_SWAPPING_UNALLOWED = 2, // Prevents memory manager from paging out memory in this segment

} MEMORY_SEGMENT_FLAGS;

typedef enum _IO_MEMORY_FLAGS {
	IO_MEMORY_CACHE_DISABLED,
	IO_MEMORY_WRITE_THROUGH,
	IO_MEMORY_READ_ONLY
} IO_MEMORY_FLAGS;

typedef struct _MEMORY_SEGMENT {
    UINT16 Flags;
    UINT16 ProcessId;
    void* BlockAddress;
    UINT64 BlockSize;
    UINT64 PageAllocationTableStart;
} MEMORY_SEGMENT;

typedef struct _MEMORY_SEGMENT_LIST_HEAD MEMORY_SEGMENT_LIST_HEAD, *RFMEMORY_SEGMENT_LIST_HEAD;



typedef struct _MEMORY_SEGMENT_LIST_HEAD {
    MEMORY_SEGMENT MemorySegments[MEMORY_LIST_HEAD_SIZE];
    RFMEMORY_SEGMENT_LIST_HEAD NextList;
} MEMORY_SEGMENT_LIST_HEAD;

#define UNALLOCATED_MEMORY_SEGMENT_CACHE_SIZE 0x100

#define NUM_FREE_MEMORY_LEVELS 4


typedef struct _MEMORY_MANAGEMENT_TABLE {
    
    UINT64 AvailableMemory; // Physical Memory + Paging File Size
    UINT64 UsedMemory;
    UINT64 PagingFileSize;
    UINT64 PagedMemory;
    UINT64 UnpageableMemory;
    UINT64 IoMemory;
    UINT64 PageAllocationTableSize; // In Number of entries
    UINT64 Reserved0; // To keep 0x10 Bytes alignment
    MEMORY_BLOCK_CACHE_LINE UnallocatedSegmentCache;
    PAGE_ALLOCATION_TABLE* PageAllocationTable;
    MEMORY_SEGMENT_LIST_HEAD AllocatedMemory;
    struct {
        MEMORY_SEGMENT_LIST_HEAD SegmentListHead;
    } FreeMemoryLevels[4];
} MEMORY_MANAGEMENT_TABLE;

extern MEMORY_MANAGEMENT_TABLE MemoryManagementTable;


typedef struct _PROCESS_MEMORY_TABLE {
    UINT64 AvailableMemory;
    UINT64 UsedMemory;
    UINT64 PagedMemory;
    UINT64 UnpageableMemory;
    MEMORY_BLOCK_CACHE_LINE UnallocatedSegmentCache; // for Allocated Memory
    MEMORY_SEGMENT_LIST_HEAD AllocatedMemory;
    struct {
        MEMORY_BLOCK_CACHE_LINE UnallocatedSegmentCache; // for Free Memory
        MEMORY_SEGMENT_LIST_HEAD SegmentListHead;
    } FreeMemoryLevels[4];
} PROCESS_MEMORY_TABLE;

#pragma pack(pop)

void InitMemoryManagementSubsystem();
RFMEMORY_SEGMENT QueryAllocatedBlockSegment(void* BlockAddress);
RFMEMORY_SEGMENT QueryFreeBlockSegment(void* BlockAddress);
RFMEMORY_SEGMENT AllocateMemorySegment(MEMORY_SEGMENT_LIST_HEAD* ListHead, MEMORY_BLOCK_CACHE_LINE* CacheLine);



typedef enum _ALLOCATE_POOL_FLAGS {
	ALLOCATE_POOL_LOW_4GB = 1,
	ALLOCATE_POOL_PHYSICAL = 2, // Returns the Physical Address of the Allocated Pool instead of its Virtual Address
	ALLOCATE_POOL_NOT_RAM = 4, // Prevents decrementing RAM When allocating IO Pool Or Virtual RAM Space POOL
    ALLOCATE_POOL_DISK_SWAPPING_UNALLOWED = 8
} ALLOCATE_POOL_FLAGS;

void* AllocatePoolEx(RFPROCESS Process, UINT64 NumBytes, UINT Align, UINT64 Flags);
void* AllocatePool(UINT64 NumBytes);
void* FreePool(void* HeapAddress);
void* RemoteFreePool(RFPROCESS Process, void* HeapAddress);

#define PAGE_FILE_LOCATION "//OS/$SwapFile"

void InitPagingFile(); // System PARTITION_INSTANCE Located in C:/ By default
KERNELSTATUS PageOutPool(RFPROCESS Process, void* VirtualAddress);
KERNELSTATUS LoadPagedPool(RFPROCESS Process, void* VirtualAddress);

LPVOID KEXPORT KERNELAPI AllocateIoMemory(_IN LPVOID PhysicalAddress, _IN UINT64 NumPages, _IN UINT Flags);
BOOL KEXPORT KERNELAPI FreeIoMemory(_IN LPVOID IoMemory);

RFMEMORY_SEGMENT (*_SIMD_AllocateMemorySegmentFromCache)(MEMORY_BLOCK_CACHE_LINE* CacheLine);
