
/*

The Operating System Memory Manager

How it works:
Page allocation table describes every page in the system
Physical Memory can only be allocated in pages.

For each process, A Virtual Memory base is assigned, and pages are allocated to the virtual mapping

The Declaration : void* AllocatePhysicalPages(RFPROCESS Process, UINT64 NumPages, void* VirtualAddressStart)

Thus the memory management subsystem allocates fragmented physical pages to the virtual memory starting at VirtualAddressStart

each process has the page allocation table start and end pointer, then the continuation is set on an allocation

*/

#pragma once
#include <krnltypes.h>
#include <mem.h>

#define ZeroMemory(Memory, Size) memset(Memory, 0, Size)
#define SZeroMemory(Memory) memset(Memory, 0, sizeof(*Memory))

#pragma pack(push, 1)

typedef struct _PROCESS_CONTROL_BLOCK *RFPROCESS;

typedef union _PAGE {
    struct {
        UINT64 Allocated : 1; // If the page is allocated to an address space
        UINT64 PresentInDisk : 1;
        UINT64 Compressed : 1;
        UINT64 Reserved : 9;
        UINT64 PhysicalPageNumber : 52;
    } PageStruct;
    UINT64 PhysicalAddress;
} PAGE;

typedef struct _PAGE_ALLOCATION_TABLE PAGE_ALLOCATION_TABLE;
typedef struct _MEMORY_SEGMENT MEMORY_SEGMENT, *RFMEMORY_SEGMENT;

#define PAGE_ALLOCATION_TABLE_ENTRY_COUNT 0x40

typedef struct _VIRTUAL_MEMORY_PAGE {
    UINT64 DiskPagingAllowed : 1;
    UINT64 CompressionAllowed : 1;
    UINT64 PagePtr : 62; // Reference to the 8-Byte aligned PAGE Structure
    MEMORY_SEGMENT* FirstHeap;
    MEMORY_SEGMENT* LastHeap;
} VIRTUAL_MEMORY_PAGE;

typedef struct _PAGE_ALLOCATION_TABLE {
    UINT64 Bitmap;
    VIRTUAL_MEMORY_PAGE VirtualMemoryPages[PAGE_ALLOCATION_TABLE_ENTRY_COUNT];
    PAGE_ALLOCATION_TABLE* Next;
} PAGE_ALLOCATION_TABLE;


#define MEMORY_BLOCK_CACHE_COUNT 4
#define MEMORY_BLOCK_CACHE_SIZE 0x40

#define MEMORY_LIST_HEAD_SIZE 0x40
#define DEFAULT_MEMORY_ALIGNMENT 0x10

#define MEMORY_SEGMENT_SEMAPHORE 2 // Bit Index 2
typedef enum _MEMORY_SEGMENT_FLAGS {
    // Initial Heap can't be relinked
    MEMORY_SEGMENT_INITIAL_HEAP = 1,
    MEMORY_SEGMENT_SWAPPING_UNALLOWED = 2, // Prevents memory manager from paging out memory in this segment
    // MEMORY_SEGMENT_SEMAPHORE = 4

} MEMORY_SEGMENT_FLAGS;

typedef enum _IO_MEMORY_FLAGS {
	IO_MEMORY_CACHE_DISABLED = 1,
	IO_MEMORY_WRITE_THROUGH = 2,
	IO_MEMORY_READ_ONLY = 4,
    IO_MEMORY_WRITE_COMBINE = 8
} IO_MEMORY_FLAGS;

typedef struct _MEMORY_SEGMENT {
    UINT32 Present : 1;
    UINT32 Reserved : 19;
    UINT32 Offset : 12;
    UINT64 HeapLength;
} MEMORY_SEGMENT;

typedef struct _FREE_MEMORY_SEGMENT {
    UINT8 Present : 1;
    UINT8 Reserved : 7;
    void* Address;
    UINT64 HeapLength;
} FREE_MEMORY_SEGMENT;


typedef struct _MEMORY_SEGMENT_LIST_HEAD MEMORY_SEGMENT_LIST_HEAD, *RFMEMORY_SEGMENT_LIST_HEAD;



typedef struct _MEMORY_SEGMENT_LIST_HEAD {
    MEMORY_SEGMENT MemorySegments[MEMORY_LIST_HEAD_SIZE];
    RFMEMORY_SEGMENT_LIST_HEAD NextListHead;
    UINT64 Bitmask;
    RFMEMORY_SEGMENT_LIST_HEAD* RefCacheLineSegment; // set if there is 
} MEMORY_SEGMENT_LIST_HEAD;

typedef struct _FREE_MEMORY_SEGMENT_LIST_HEAD {
    FREE_MEMORY_SEGMENT MemorySegments[MEMORY_LIST_HEAD_SIZE];
    RFMEMORY_SEGMENT_LIST_HEAD NextListHead;
    UINT64 Bitmask;
    RFMEMORY_SEGMENT_LIST_HEAD* RefCacheLineSegment; // set if there is 
} FREE_MEMORY_SEGMENT_LIST_HEAD;


#define NUM_FREE_MEMORY_LEVELS 4

typedef enum _FREE_MEMORY_LEVELS {
    FREE_MEMORY_BELOW_1KB = 0,
    FREE_MEMORY_BELOW_64KB = 1,
    FREE_MEMORY_BELOW_2MB = 2,
    FREE_MEMORY_ABOVE_2MB = 3 
} FREE_MEMORY_LEVELS;

// Used to cache and optimize memory functions
typedef struct _MEMORY_REGION_TABLE {
    // Variables are ordered this way to keep 0x10 Bytes alignment and therefore win cpu clocks
    MEMORY_SEGMENT_LIST_HEAD SegmentListHead;
    UINT64 AllocatedEntries;
    UINT64 TotalEntries;
    RFMEMORY_SEGMENT_LIST_HEAD LastListHead; // To optimize fetching, when :
                                            // AllocatedEntries = Total Entries
                                            /// LastListHead->Next = New List Head
    UINT64 AllocateListControl; // UINT64 To make alignement
} MEMORY_REGION_TABLE, *RFMEMORY_REGION_TABLE;


// Global Memory Management Table for kernel processes and allocation source for user processes
typedef struct _MEMORY_MANAGEMENT_TABLE {
    UINT64 TotalMemory;
    UINT64 PhysicalMemory;
    UINT64 TotalDiskMemory;
    UINT64 TotalCompressedMemory; // Compressed physical memory
    UINT64 UsedPhysicalMemory;
    UINT64 UsedDiskMemory;
    UINT64 IoMemorySize;
    UINT64 PageArraySize; // In Number of entries
    
    PAGE* PageArray;
    UINT64 NumBytesPageBitmap;
    char* PageBitmap;
} MEMORY_MANAGEMENT_TABLE;

extern MEMORY_MANAGEMENT_TABLE MemoryManagementTable;


// Only for user processes
typedef struct _PROCESS_MEMORY_TABLE {
    UINT64 AllocatedPages;
    UINT64 ReservedPages;
    UINT64 CompressedPages;
    UINT64 DiskPages;

    UINT64 UsedMemory; // Used memory from these reserved pages
    PAGE_ALLOCATION_TABLE PageAllocationTable;

    MEMORY_SEGMENT_LIST_HEAD AllocatedMemory;
    FREE_MEMORY_SEGMENT_LIST_HEAD FreeMemory[NUM_FREE_MEMORY_LEVELS];
} PROCESS_MEMORY_TABLE;

#pragma pack(pop)

void InitMemoryManagementSubsystem();
RFMEMORY_SEGMENT QueryAllocatedBlockSegment(void* BlockAddress);
RFMEMORY_SEGMENT QueryFreeBlockSegment(void* BlockAddress);
RFMEMORY_SEGMENT AllocateMemorySegment(RFMEMORY_REGION_TABLE MemoryRegion);

typedef enum _ALLOCATE_POOL_FLAGS {
	ALLOCATE_POOL_LOW_4GB = 1,
	ALLOCATE_POOL_PHYSICAL = 2, // Returns the Physical Address of the Allocated Pool instead of its Virtual Address
	ALLOCATE_POOL_NOT_RAM = 4, // Prevents decrementing RAM When allocating IO Pool Or Virtual RAM Space POOL
    ALLOCATE_POOL_DISK_SWAPPING_UNALLOWED = 8,
    // Only used by memory manager to prevent a deadlock when allocating a new list head
    // when set the memory manager should check if it's in heap segments, if yes then
    // it will allocate the new heap segment in this list, otherwise it will work normally
    ALLOCATE_POOL_NEW_LIST_HEAD = 0x10
} ALLOCATE_POOL_FLAGS;


// The system will Set a PANIC_SCREEN if return value is 0 and the process is a kernel mode process
void* AllocatePoolEx(RFPROCESS Process, UINT64 NumBytes, UINT Align, UINT64 Flags);
void* AllocatePool(UINT64 NumBytes);
void* FreePool(void* HeapAddress);
void* RemoteFreePool(RFPROCESS Process, void* HeapAddress);

// Process = NULL will return true physical pages, only allowed before system startup
void* AllocateContiguousPages(RFPROCESS Process, UINT64 NumPages, UINT64 Flags);

#define PAGE_FILE_LOCATION "//OS/$SwapFile"

void InitPagingFile(); // System PARTITION_INSTANCE Located in C:/ By default
KERNELSTATUS PageOutPool(RFPROCESS Process, void* VirtualAddress);
KERNELSTATUS LoadPagedPool(RFPROCESS Process, void* VirtualAddress);

LPVOID KEXPORT KERNELAPI AllocateIoMemory(_IN LPVOID PhysicalAddress, _IN UINT64 NumPages, _IN UINT Flags);
BOOL KEXPORT KERNELAPI FreeIoMemory(_IN LPVOID IoMemory);



// The free memory segment has the semaphore bit set until the pool is actually given
// the host routine must reset the MEMORY_SEGMENT_SEMAPHORE Bit in the returned Segment
RFMEMORY_SEGMENT MemMgr_FreePool(RFMEMORY_REGION_TABLE MemoryRegion, RFMEMORY_SEGMENT_LIST_HEAD ListHead, RFMEMORY_SEGMENT MemorySegment);
RFMEMORY_SEGMENT MemMgr_CreateInitialHeap(void* HeapAddress, UINT64 HeapLength);

// Sets present bit in Memory Segment flags when Found
RFMEMORY_SEGMENT (*_SIMD_FetchUnusedSegmentsUncached)(MEMORY_SEGMENT_LIST_HEAD* ListHead);
LPVOID (__fastcall *_SIMD_AllocatePhysicalPage) (char* PageBitmap, UINT64 BitmapSize, PAGE* PageArray);
// ---------------------------

void SIMD_InitOptimizedMemoryManagement();