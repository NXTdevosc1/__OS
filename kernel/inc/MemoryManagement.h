#pragma once
#include <krnltypes.h>
#include <mem.h>
typedef struct _MEMORY_MANAGEMENT_TABLE MEMORY_MANAGEMENT_TABLE;

typedef struct _MEMORYSTATUS {
	UINT16 MemoryLoad;
	UINT64 VirtualMemoryLoad;
	UINT64 TotalPhys;
	UINT64 AvailaiblePhys;
	UINT64 TotalPageFile;
	UINT64 AvailaiblePageFile;
	UINT64 TotalVirtual;
	UINT64 RemainingVirtual;
} MEMORYSTATUS, * LPMEMORYSTATUS;

typedef struct _MEMORY_HEAP_DESCRIPTOR MEMORY_HEAP_DESCRIPTOR, * PMEMORY_HEAP_DESCRIPTOR;


typedef struct _FREE_MEMORY_LIST FREE_MEMORY_LIST;
typedef struct _ALLOCATED_MEMORY_LIST ALLOCATED_MEMORY_LIST;

typedef struct _FREE_HEAP_DESCRIPTOR {
	BOOL Present;
	UINT Index;
	FREE_MEMORY_LIST* ParentList;
	PMEMORY_HEAP_DESCRIPTOR PhysicalHeap;
	UINTPTR Address;
	UINT64 HeapLength;
	UINT64 UsedMemory;
} FREE_HEAP_DESCRIPTOR, * PFREE_HEAP_DESCRIPTOR;

typedef struct _MEMORY_HEAP_DESCRIPTOR {
	BOOL Present; // Present == 3 Means that it's a private heap
	UINT Index;
	LPVOID Thread;
	ALLOCATED_MEMORY_LIST* ParentList;

	PFREE_HEAP_DESCRIPTOR SourceHeap; // When heap slices in user process, it maybe unrecoverable on free until process is terminated
	UINTPTR Address;
	UINT64 HeapLength;
} MEMORY_HEAP_DESCRIPTOR, * PMEMORY_HEAP_DESCRIPTOR;

typedef struct _FREE_MEMORY_LIST {
	UINT SearchMax;
	UINT HeapCount;
	FREE_HEAP_DESCRIPTOR Heaps[UNITS_PER_LIST];
	FREE_MEMORY_LIST* Next;
} FREE_MEMORY_LIST;

typedef struct _ALLOCATED_MEMORY_LIST {
	UINT SearchMax;
	UINT HeapCount;
	MEMORY_HEAP_DESCRIPTOR Heaps[UNITS_PER_LIST];
	ALLOCATED_MEMORY_LIST* Next;
} ALLOCATED_MEMORY_LIST;

typedef struct _MEMORY_MANAGEMENT_TABLE {
	LPVOID Process;
	UINT64 PhysicalUsedMemory; // AvailaibleMemory + PrivateBytes
	UINT64 AvailaibleMemory;
	UINT64 UsedMemory;
	ALLOCATED_MEMORY_LIST* AllocatedMemory; // Last Allocated mem list
	FREE_MEMORY_LIST* FreeMemory; // last free mem list
	ALLOCATED_MEMORY_LIST AllocatedMemoryList;
	FREE_MEMORY_LIST FreeMemoryList;
} MEMORY_MANAGEMENT_TABLE;

typedef struct _PROCESS_CONTROL_BLOCK PROCESS, *RFPROCESS;
typedef struct _THREAD_CONTROL_BLOCK THREAD, *HTHREAD;

typedef enum _IO_MEMORY_FLAGS {
	IO_MEMORY_CACHE_DISABLED,
	IO_MEMORY_WRITE_THROUGH,
	IO_MEMORY_READ_ONLY
} IO_MEMORY_FLAGS;

BOOL KERNELAPI CreateMemoryTable(RFPROCESS Process, MEMORY_MANAGEMENT_TABLE* MemoryTable);

PMEMORY_HEAP_DESCRIPTOR KERNELAPI AllocateHeapDescriptor(
	MEMORY_MANAGEMENT_TABLE* MemoryTable,
	HTHREAD Thread, 
	PFREE_HEAP_DESCRIPTOR SourceHeap,
	LPVOID Address,
	UINT64 HeapLength
);


PFREE_HEAP_DESCRIPTOR KERNELAPI AllocateFreeHeapDescriptor(
	RFPROCESS Process,
	PMEMORY_HEAP_DESCRIPTOR PhysicalHeap, 
	LPVOID Address, 
	UINT64 HeapLength
);

LPVOID KERNELAPI HeapCreate(MEMORY_MANAGEMENT_TABLE* MemoryTable, UINT64* NumBytes, UINT32 Align, PFREE_HEAP_DESCRIPTOR* SourceHeap, UINT64 MaxAddress); // Search for a heap

LPVOID KEXPORT KERNELAPI AllocatePoolEx(HTHREAD Thread, UINT64 NumBytes, UINT32 Align, LPVOID* AllocationSegment, UINT64 MaxAddress);


LPVOID KEXPORT KERNELAPI malloc(UINT64 NumBytes);
LPVOID KEXPORT KERNELAPI free(LPVOID Heap, RFPROCESS Process);
LPVOID KEXPORT KERNELAPI FastFree(LPVOID AllocationSegment, RFPROCESS Process);
LPVOID KERNELAPI kfree(LPVOID Heap);

LPVOID KERNELAPI kmalloc(UINT64 NumBytes);
LPVOID KERNELAPI kpalloc(UINT64 NumPages);

LPVOID KERNELAPI UserExtendedAlloc(UINT64 NumBytes, UINT32 Align);
LPVOID KERNELAPI UserMalloc(UINT64 NumBytes);

LPVOID KERNELAPI AllocateUserHeapSpace(RFPROCESS Process, UINT64 NumBytes); // Automatically Aligned to 0x1000

BOOL GetPhysicalMemoryStatus(LPMEMORYSTATUS MemoryStatus);

LPVOID KERNELAPI CurrentProcessMalloc(unsigned long long HeapSize);
LPVOID KERNELAPI CurrentProcessFree(LPVOID Heap);

typedef struct _GLOBAL_MEMORY_STATUS {
	UINT64 TotalMemory;
	UINT64 AllocatedMemory;
} PHYSICALMEMORYSTATUS, * LPPHYSICALMEMORYSTATUS;

extern PHYSICALMEMORYSTATUS PhysicalMemoryStatus;

LPVOID KEXPORT KERNELAPI AllocateIoMemory(LPVOID PhysicalAddress, UINT64 NumPages, UINT Flags);
BOOL KEXPORT KERNELAPI FreeIoMemory(LPVOID IoMem);