
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

#define PAGE_SIZE 0x1000

#pragma once
#include <krnltypes.h>
#include <mem.h>

#define ZeroMemory(Memory, Size) memset((void*)Memory, 0, Size)
#define SZeroMemory(Memory) memset((void*)Memory, 0, sizeof(*Memory))


typedef struct _FREE_MEMORY_TREE FREE_MEMORY_TREE;

#define MEMTREE_MAX_SLOTS 128
#define MEMTREE_NUM_SLOT_GROUPS 2

typedef volatile struct _FREE_MEMORY_SEGMENT FREE_MEMORY_SEGMENT;



typedef volatile struct _FREE_MEMORY_SEGMENT {
    void* Address;
    UINT64 HeapLength;
    UINT64 InitialLength : 63;
    UINT64 InitialHeap : 1; // This heap can only be deleted if the page is released
} FREE_MEMORY_SEGMENT;

typedef volatile struct _FREE_MEMORY_SEGMENT_LIST_HEAD FREE_MEMORY_SEGMENT_LIST_HEAD;

typedef struct _MEMORY_TREE_ITEM {
    void* Child; // FREE_MEMORY_TREE or HEAP_LIST
    volatile UINT32 LockMutex;
    FREE_MEMORY_SEGMENT* LargestHeap;
    FREE_MEMORY_SEGMENT_LIST_HEAD* ListHeadLargestHeap;
    UINT8 IndexLargestHeap;
} MEMORY_TREE_ITEM;

typedef struct _FREE_MEMORY_TREE {
    UINT8 FullSlotCount;
    UINT ParentItemIndex;
    FREE_MEMORY_TREE* Parent;
    UINT64 Present[MEMTREE_NUM_SLOT_GROUPS];
    UINT64 FullSlots[MEMTREE_NUM_SLOT_GROUPS];
    UINT8 PresentSlotCount;

    MEMORY_TREE_ITEM Childs[MEMTREE_MAX_SLOTS];


    FREE_MEMORY_TREE* Next; // only in level1 tree
    BOOL Root;
    UINT8 Rsv[322];
} FREE_MEMORY_TREE;

// 3rd level of the tree
typedef volatile struct _FREE_MEMORY_SEGMENT_LIST_HEAD {
    UINT8 NumUsedSlots;
    UINT ParentItemIndex;
    FREE_MEMORY_TREE* Parent;
    UINT64 UsedSlots[MEMTREE_NUM_SLOT_GROUPS];
    FREE_MEMORY_SEGMENT MemorySegments[MEMTREE_MAX_SLOTS];
} FREE_MEMORY_SEGMENT_LIST_HEAD;



typedef volatile struct _PROCESS_CONTROL_BLOCK *RFPROCESS;

typedef volatile union _PAGE {
    struct {
        UINT64 Allocated : 1; // If the page is allocated to an address space
        UINT64 PresentInDisk : 1;
        UINT64 Compressed : 1;
        UINT64 Reserved : 9;
        UINT64 PhysicalPageNumber : 52;
    } PageStruct;
    UINT64 PhysicalAddress;
} PAGE;

typedef volatile struct _PAGE_ALLOCATION_TABLE PAGE_ALLOCATION_TABLE;
typedef volatile struct _MEMORY_SEGMENT MEMORY_SEGMENT, *RFMEMORY_SEGMENT;

#define PAGE_ALLOCATION_TABLE_ENTRY_COUNT 0x80

typedef volatile struct _VIRTUAL_MEMORY_PAGE {
    UINT64 DiskPagingAllowed : 1;
    UINT64 CompressionAllowed : 1;
    UINT64 PagePtr : 62; // Reference to the 8-Byte aligned PAGE Structure
    MEMORY_SEGMENT* FirstHeap;
    MEMORY_SEGMENT* LastHeap;

} VIRTUAL_MEMORY_PAGE;

// All structures must fit inside a page
typedef volatile struct _PAGE_ALLOCATION_TABLE {
    UINT64* Bitmap;
    UINT64 Offset;
    UINT64 AllocatedPages;
    
    VIRTUAL_MEMORY_PAGE VirtualMemoryPages[0x40];
    PAGE_ALLOCATION_TABLE* Next;
} PAGE_ALLOCATION_TABLE;

typedef volatile struct _PAGE_ALLOCATION_BITMAP PAGE_ALLOCATION_BITMAP;

// each bit specifies if there is a free page in an allocation table
typedef volatile struct _PAGE_ALLOCATION_BITMAP {
    UINT64 Bitmap[0x1FF];
    PAGE_ALLOCATION_BITMAP* Next;
} PAGE_ALLOCATION_BITMAP;

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

typedef volatile struct _MEMORY_SEGMENT {
    UINT32 Present : 1;
    UINT32 Reserved : 19;
    UINT32 Offset : 12;
    UINT64 HeapLength;
} MEMORY_SEGMENT;



typedef volatile struct _MEMORY_SEGMENT_LIST_HEAD MEMORY_SEGMENT_LIST_HEAD, *RFMEMORY_SEGMENT_LIST_HEAD;




typedef volatile struct _MEMORY_SEGMENT_LIST_HEAD {
    MEMORY_SEGMENT MemorySegments[MEMORY_LIST_HEAD_SIZE];
    RFMEMORY_SEGMENT_LIST_HEAD NextListHead;
    UINT64 Bitmask;
    RFMEMORY_SEGMENT_LIST_HEAD* RefCacheLineSegment; // set if there is 
} MEMORY_SEGMENT_LIST_HEAD;



#pragma pack(push, 1)

// Global Memory Management Table for kernel processes and allocation source for user processes
typedef volatile struct _MEMORY_MANAGEMENT_TABLE {
    UINT64 TotalPages;
    UINT64 PhysicalPages;
    UINT64 DiskPages;
    UINT64 CompressedPages; // Compressed physical memory
    UINT64 AllocatedPages;
    UINT64 AllocatedDiskPages;
    UINT64 IoMemorySize;
    UINT64 PageArraySize; // In Number of entries
    
    PAGE* PageArray; // 0x40
    UINT64 NumBytesPageBitmap; // 0x48
    char* PageBitmap; // 0x50
    char* FreePagesStart; // Offset 0x58
    PAGE* FreePageArrayStart; // Offset 0x60
    char* BmpEnd; // Off 0x68
} MEMORY_MANAGEMENT_TABLE;

#pragma pack(pop)

extern MEMORY_MANAGEMENT_TABLE MemoryManagementTable;


// Only for user processes
typedef struct _PROCESS_MEMORY_TABLE {
    void* VirtualBase;
    UINT64 AllocatedPages;
    UINT64 ReservedPages;
    UINT64 CompressedPages;
    UINT64 DiskPages;
    UINT64 UsedMemory; // Used memory from these reserved pages
    FREE_MEMORY_TREE FreeMemory;
} PROCESS_MEMORY_TABLE;


void InitMemoryManagementSubsystem(void);
RFMEMORY_SEGMENT QueryAllocatedBlockSegment(void* BlockAddress);
RFMEMORY_SEGMENT QueryFreeBlockSegment(void* BlockAddress);

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

// KERNELSTATUS KERNELAPI AllocateVirtualMemory(RFPROCESS Process, VIRTUAL_MEMORY_PAGE* )

#define PAGE_FILE_LOCATION "//OS/$SwapFile"

void InitPagingFile(void); // System PARTITION_INSTANCE Located in C:/ By default
KERNELSTATUS PageOutPool(RFPROCESS Process, void* VirtualAddress);
KERNELSTATUS LoadPagedPool(RFPROCESS Process, void* VirtualAddress);

LPVOID KEXPORT KERNELAPI AllocateIoMemory(_IN LPVOID PhysicalAddress, _IN UINT64 NumPages, _IN UINT Flags);
BOOL KEXPORT KERNELAPI FreeIoMemory(_IN LPVOID IoMemory);


void KEXPORT *KERNELAPI KeAllocateVirtualMemory(UINT64 NumBytes);

BOOL KeAllocateFragmentedPages(RFPROCESS Process, void* VirtualMemory, UINT64 NumPages);

// The free memory segment has the semaphore bit set until the pool is actually given
// the host routine must reset the MEMORY_SEGMENT_SEMAPHORE Bit in the returned Segment

// Sets present bit in Memory Segment flags when Found
RFMEMORY_SEGMENT (*_SIMD_FetchUnusedSegmentsUncached)(MEMORY_SEGMENT_LIST_HEAD* ListHead);
LPVOID (__fastcall *_SIMD_AllocatePhysicalPage) ();
// ---------------------------

void SIMD_InitOptimizedMemoryManagement(void);