#include <MemoryManagement.h>
#include <mem.h>
#include <interrupt_manager/SOD.h>
#include <kernel.h>
#include <CPU/paging.h>
#include <CPU/process.h>
#include <preos_renderer.h>
#include <CPU/cpu.h>

__declspec(align(0x1000)) PHYSICALMEMORYSTATUS PhysicalMemoryStatus = { 0 };


BOOL KERNELAPI CreateMemoryTable(RFPROCESS Process, MEMORY_MANAGEMENT_TABLE* MemoryTable) {
	if (!Process || !MemoryTable) return FALSE;
	SZeroMemory(MemoryTable);
	MemoryTable->Process = Process;
	MemoryTable->AllocatedMemory = &MemoryTable->AllocatedMemoryList;
	MemoryTable->FreeMemory = &MemoryTable->FreeMemoryList;
	return TRUE;
}



PMEMORY_HEAP_DESCRIPTOR KERNELAPI AllocateHeapDescriptor(
	MEMORY_MANAGEMENT_TABLE* MemoryTable,
	HTHREAD Thread,
	PFREE_HEAP_DESCRIPTOR SourceHeap,
	LPVOID Address,
	UINT64 HeapLength
) {
	if (!MemoryTable || !Address || !HeapLength) return NULL;

	ALLOCATED_MEMORY_LIST* List = MemoryTable->AllocatedMemory;
	for (;;) {
		if (List->HeapCount == UNITS_PER_LIST) goto NextList;
		if (List->SearchMax < UNITS_PER_LIST) {
			// Found A Heap
			PMEMORY_HEAP_DESCRIPTOR Heap = &List->Heaps[List->SearchMax];
			Heap->Present = TRUE;
			Heap->ParentList = List;
			Heap->Address = (UINTPTR)Address;
			Heap->SourceHeap = SourceHeap;
			Heap->Thread = Thread;
			Heap->HeapLength = HeapLength;
			Heap->Index = List->SearchMax;

			List->SearchMax++;
			List->HeapCount++;
			return Heap;
		}
		for (int i = 0; i < UNITS_PER_LIST; i++) {
			if (!List->Heaps[i].Present) {
				PMEMORY_HEAP_DESCRIPTOR Heap = &List->Heaps[i];
				Heap->Present = TRUE;
				Heap->ParentList = List;
				Heap->Address = (UINTPTR)Address;
				Heap->SourceHeap = SourceHeap;
				Heap->Thread = Thread;
				Heap->HeapLength = HeapLength;
				Heap->Index = i;
				List->HeapCount++;
				return Heap;
			}
		}
	NextList:
		if (!List->Next) {
			PFREE_HEAP_DESCRIPTOR ListSourceHeap = NULL;
			UINT64 ListHeapLength = sizeof(ALLOCATED_MEMORY_LIST) + 0x100;
			
			List = (ALLOCATED_MEMORY_LIST*)HeapCreate(MemoryTable, &ListHeapLength, 0x1000, &ListSourceHeap, 0);
			if (!List) SET_SOD_MEMORY_MANAGEMENT;
			SZeroMemory(List);
			
			List->Next = MemoryTable->AllocatedMemory;
			MemoryTable->AllocatedMemory = List;

			PMEMORY_HEAP_DESCRIPTOR ListHeap = &List->Heaps[0];
			PMEMORY_HEAP_DESCRIPTOR Heap = &List->Heaps[1];

			ListHeap->Present = TRUE | 2; // Set Private Flag
			ListHeap->Address = (UINTPTR)List;
			ListHeap->HeapLength = ListHeapLength;
			ListHeap->ParentList = List;
			ListHeap->Thread = NULL;
			ListHeap->SourceHeap = ListSourceHeap;

			Heap->Present = TRUE;
			Heap->ParentList = List;
			Heap->Address = (UINTPTR)Address;
			Heap->SourceHeap = SourceHeap;
			Heap->Thread = Thread;
			Heap->HeapLength = HeapLength;
			Heap->Index = 1;

			List->HeapCount = 2;
			List->SearchMax = 2;
			return Heap;
		}
		List = List->Next;
	}
	return NULL;
}




PFREE_HEAP_DESCRIPTOR KERNELAPI AllocateFreeHeapDescriptor(
	RFPROCESS Process,
	PMEMORY_HEAP_DESCRIPTOR PhysicalHeap,
	LPVOID Address,
	UINT64 HeapLength
) {
	if (!Process) return NULL;
	__SpinLockSyncBitTestAndSet(&Process->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_FREE_HEAP_DESCRIPTOR);
	FREE_MEMORY_LIST* List = Process->MemoryManagementTable.FreeMemory;
	for (;;) {
		if (List->HeapCount == UNITS_PER_LIST) goto NextList;
		if (List->SearchMax < UNITS_PER_LIST) {
			PFREE_HEAP_DESCRIPTOR Descriptor = &List->Heaps[List->SearchMax];

			Descriptor->Present = TRUE;
			Descriptor->Index = List->SearchMax;
			Descriptor->ParentList = List;
			Descriptor->PhysicalHeap = PhysicalHeap;
			Descriptor->Address = (UINTPTR)Address;
			Descriptor->HeapLength = HeapLength;

			List->HeapCount++;
			List->SearchMax++;
			__BitRelease(&Process->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_FREE_HEAP_DESCRIPTOR);
			return Descriptor;
		}
		for (int i = 0; i < List->SearchMax; i++) {
			if (!List->Heaps[i].Present) {
				PFREE_HEAP_DESCRIPTOR Descriptor = &List->Heaps[i];
				Descriptor->Present = TRUE;
				Descriptor->Index = i;
				Descriptor->ParentList = List;
				Descriptor->PhysicalHeap = PhysicalHeap;
				Descriptor->Address = (UINTPTR)Address;
				Descriptor->HeapLength = HeapLength;
				List->HeapCount++;
				__BitRelease(&Process->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_FREE_HEAP_DESCRIPTOR);
				return Descriptor;
			}
		}
	NextList:
		if (!List->Next) {
			UINT64 ListBytes = sizeof(FREE_MEMORY_LIST) + 0x100;
			// PMEMORY_HEAP_DESCRIPTOR ListHeap = NULL;
			List = AllocatePoolEx(NULL, ListBytes, 0x1000,  0);
			
			if (!List) SET_SOD_MEMORY_MANAGEMENT;
			SZeroMemory(List);

			List->Next = Process->MemoryManagementTable.FreeMemory;
			Process->MemoryManagementTable.FreeMemory = List;
			// ListHeap->Present |= 2; // Set Private Heap Flag
			PFREE_HEAP_DESCRIPTOR Descriptor = &List->Heaps[0];


			Descriptor->Present = TRUE;
			Descriptor->ParentList = List;
			Descriptor->PhysicalHeap = PhysicalHeap;
			Descriptor->Address = (UINTPTR)Address;
			Descriptor->HeapLength = HeapLength;
			List->HeapCount++;
			List->SearchMax++;
			__BitRelease(&Process->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_FREE_HEAP_DESCRIPTOR);
			return Descriptor;
		}
		List = List->Next;
	}
	__BitRelease(&Process->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_FREE_HEAP_DESCRIPTOR);
	return NULL;
}

LPVOID KERNELAPI HeapCreate(MEMORY_MANAGEMENT_TABLE* MemoryTable, UINT64* NumBytes, UINT32 Align, PFREE_HEAP_DESCRIPTOR* SourceHeap, UINT64 MaxAddress) {
	if (!MemoryTable || !SourceHeap || !NumBytes || !*NumBytes) return NULL;
	if(!MaxAddress) MaxAddress = (UINT64)-1;
	if (Align < 0x10) Align = 0x10; // Force 16 Byte Align
	if (*NumBytes % 0x10) *NumBytes += 0x10 - (*NumBytes % 0x10);
	FREE_MEMORY_LIST* List = MemoryTable->FreeMemory;

	FREE_HEAP_DESCRIPTOR* Heap = NULL;
	for (;;) {

		if (!List->HeapCount) goto NextList;
		for (int i = List->SearchMax - 1; i >= 0;i--) {
			Heap = &List->Heaps[i];
			if (Heap->Present) {
				UINT64 EstimatedSize = *NumBytes;
				UINTPTR Address = Heap->Address + Heap->UsedMemory;
				if(Address + *NumBytes > MaxAddress) continue;
				if ((Heap->Address + Heap->UsedMemory) % Align) {
					EstimatedSize += Align - ((Heap->Address + Heap->UsedMemory) % Align);
					Address += Align - ((Heap->Address + Heap->UsedMemory) % Align);
				}
				if (Heap->HeapLength - Heap->UsedMemory < EstimatedSize) continue;
				Heap->UsedMemory += EstimatedSize;
				*SourceHeap = Heap;
				if (Heap->UsedMemory == Heap->HeapLength) {
					*SourceHeap = NULL;
					Heap->Present = FALSE;
				}

				*NumBytes = EstimatedSize;
				MemoryTable->PhysicalUsedMemory += EstimatedSize;
				PhysicalMemoryStatus.AllocatedMemory += EstimatedSize;
				return (LPVOID)Address;
			}
		}

		NextList:
		if (!List->Next) break;

		List = List->Next;
	}
	return NULL;
}

LPVOID KERNELAPI AllocatePoolEx(HTHREAD Thread, UINT64 NumBytes, UINT32 Align, UINT64 Flags){
	if (!NumBytes) return NULL;
	RFPROCESS Process = NULL;
	if (Thread) Process = Thread->Process;
	else Process = KeGetCurrentProcess();
	PFREE_HEAP_DESCRIPTOR SourceHeap = NULL;
	if (Process->OperatingMode == KERNELMODE_PROCESS) Process = kproc;
	UINT64 MaxAddress = 0;
	if(!(Flags & ALLOCATE_POOL_PHYSICAL)) {
		// Virtual pool must be Page Aligned
		if((NumBytes & 0xFFF) || Align != 0x1000) return NULL;
	}
	if(Flags & ALLOCATE_POOL_LOW_4GB) {
		if(NumBytes >= 0xFFFFFFFF) return NULL;
		MaxAddress = 0xFFFFFFFF - NumBytes;
	}
	LPVOID Heap = HeapCreate(&Process->MemoryManagementTable, &NumBytes, Align, &SourceHeap, MaxAddress);
	if (!Heap) {
		if (Process == kproc) SET_SOD_MEMORY_MANAGEMENT;// Kernel mode process
		
		// TODO: Allocate New Heap For User Process

	}
	PMEMORY_HEAP_DESCRIPTOR Descriptor = AllocateHeapDescriptor(&Process->MemoryManagementTable,
		Thread, SourceHeap, Heap, NumBytes
	);
	if(!(Flags & ALLOCATE_POOL_NOT_RAM)) {
		Process->MemoryManagementTable.UsedMemory += NumBytes;
	}
	if(!(Flags & ALLOCATE_POOL_PHYSICAL)) {
		LPVOID VirtualAllocated = AllocatePoolEx(&VirtualRamThread, NumBytes, 0, ALLOCATE_POOL_PHYSICAL | ALLOCATE_POOL_NOT_RAM);
		if(!VirtualAllocated) {
			free(Heap, Process);
			return NULL;
		}
		KeMapMemory(Heap, VirtualAllocated, NumBytes >> 12, PM_MAP);
		Heap = VirtualAllocated;
	}


	return Heap;
}

LPVOID KERNELAPI malloc(UINT64 NumBytes) {
	return AllocatePoolEx(KeGetCurrentThread(), NumBytes, 0, ALLOCATE_POOL_PHYSICAL);
}

LPVOID kmalloc(UINT64 NumBytes) {
	return AllocatePoolEx(NULL, NumBytes, 0, ALLOCATE_POOL_PHYSICAL);
}
LPVOID KERNELAPI kpalloc(UINT64 NumPages) {
	return AllocatePoolEx(NULL, NumPages << 12, 0x1000, 0);
}

LPVOID KERNELAPI FastFree(LPVOID AllocationSegment, RFPROCESS Process) {
	if (!AllocationSegment) return NULL;
	
	PMEMORY_HEAP_DESCRIPTOR Descriptor = AllocationSegment;
	HTHREAD Thread = Descriptor->Thread;
	RFPROCESS DescriptorProcess = KeGetCurrentProcess();
	if (Thread) DescriptorProcess = Thread->Process;

	if (!Descriptor->Present || !Descriptor->ParentList || DescriptorProcess != Process) return NULL;
	if (&Descriptor->ParentList->Heaps[Descriptor->Index] != Descriptor) return NULL;
	PFREE_HEAP_DESCRIPTOR FreeHeapDesc = AllocateFreeHeapDescriptor(
		Process, NULL, (LPVOID)Descriptor->Address, Descriptor->HeapLength
	);
	if (!FreeHeapDesc) SET_SOD_MEMORY_MANAGEMENT;
	Thread->Process->MemoryManagementTable.PhysicalUsedMemory -= Descriptor->HeapLength;
	if (!(Descriptor->Present & 2)) {
		Thread->Process->MemoryManagementTable.UsedMemory -= Descriptor->HeapLength;
	}
	PhysicalMemoryStatus.AllocatedMemory -= Descriptor->HeapLength;
	LPVOID ReturnAddress = (LPVOID)Descriptor->Address;
	Descriptor->ParentList->HeapCount--;
	if (Descriptor->ParentList->SearchMax + 1 == Descriptor->Index) Descriptor->ParentList->SearchMax--;
	Descriptor->Present = FALSE;
	return ReturnAddress;
}

LPVOID KERNELAPI UserExtendedAlloc(UINT64 NumBytes, UINT32 Align) {
	return NULL;
}
LPVOID KERNELAPI UserMalloc(UINT64 NumBytes) {
	return NULL;
}


LPVOID KERNELAPI free(LPVOID Heap, RFPROCESS Process) {
	ALLOCATED_MEMORY_LIST* List = Process->MemoryManagementTable.AllocatedMemory;
	ALLOCATED_MEMORY_LIST* LastList = List;
	if(!Heap) return NULL;
	if((UINT64)Heap & (UINT64)CANONICAL_SIGN_EXTEND48) {
		Heap = KeResolvePhysicalAddress(Process, Heap);
		if(!Heap) return NULL;
	}
	for (;;) {
		if (!List->HeapCount) {
			// Unlink List And Lower memory usage
			if (LastList != List) {
				LastList->Next = List->Next;
				kfree((LPVOID)List);
				List = LastList;
			}
			goto NextList;
		}
		for (int i = List->SearchMax; i >= 0; i--) {
			if (List->Heaps[i].Address == (UINTPTR)Heap && List->Heaps[i].Present) {
				PMEMORY_HEAP_DESCRIPTOR Descriptor = &List->Heaps[i];

				PFREE_HEAP_DESCRIPTOR FreeHeapDesc = AllocateFreeHeapDescriptor(
					Process, NULL, (LPVOID)Descriptor->Address, Descriptor->HeapLength
				);
				if (!FreeHeapDesc) SET_SOD_MEMORY_MANAGEMENT;
				
				Process->MemoryManagementTable.PhysicalUsedMemory -= Descriptor->HeapLength;
				PhysicalMemoryStatus.AllocatedMemory -= Descriptor->HeapLength;
				if (!(Descriptor->Present & 2)) // Heap is not private
				{
					Process->MemoryManagementTable.UsedMemory -= Descriptor->HeapLength;
				}

				LPVOID ReturnAddress = (LPVOID)Descriptor->Address;
				Descriptor->ParentList->HeapCount--;
				if (Descriptor->ParentList->SearchMax + 1 == Descriptor->Index) Descriptor->ParentList->SearchMax--;

				Descriptor->Present = FALSE;
				return ReturnAddress;
			}
		}
	NextList:
		if (!List->Next) break;
		List = List->Next;
		LastList = List;
	}
	return NULL;
}
LPVOID KERNELAPI kfree(LPVOID Heap) {
	return free(Heap, kproc);
}

LPVOID KERNELAPI CurrentProcessMalloc(unsigned long long HeapSize) {
	HTHREAD Thread = KeGetCurrentThread();
	return AllocatePoolEx(Thread, HeapSize, 0, 0);
}
LPVOID KERNELAPI CurrentProcessFree(LPVOID Heap) {
	return free(Heap, KeGetCurrentProcess());
}

LPVOID KERNELAPI AllocateUserHeapSpace(RFPROCESS Process, UINT64 NumBytes) {
	// if (!Process || Process->OperatingMode != USERMODE_PROCESS) return NULL;
	// if (!NumBytes) NumBytes = 0x1000;

	// if (NumBytes % 0x1000) NumBytes += 0x1000 - (NumBytes % 0x1000);
	// PMEMORY_HEAP_DESCRIPTOR SourceHeap = NULL;
	// LPVOID Buffer = AllocatePoolEx(NULL, NumBytes, 0x1000, (LPVOID*)&SourceHeap, 0);
	// if (!Buffer || !SourceHeap) SET_SOD_MEMORY_MANAGEMENT;

	
	// PFREE_HEAP_DESCRIPTOR Heap = AllocateFreeHeapDescriptor(Process, SourceHeap, (LPVOID)(USER_HEAP_BASE + Process->MemoryManagementTable.AvailaibleMemory), SourceHeap->HeapLength);
	// if (!Heap) SET_SOD_MEMORY_MANAGEMENT;
	// MapPhysicalPages(Process->PageMap, (LPVOID)(USER_HEAP_BASE + Process->MemoryManagementTable.AvailaibleMemory), Buffer, NumBytes >> 12, PM_UMAP);
	// Process->MemoryManagementTable.AvailaibleMemory += SourceHeap->HeapLength;
	// return Buffer;
	return NULL;
}


BOOL GetPhysicalMemoryStatus(LPMEMORYSTATUS MemoryStatus) {
	return FALSE;
}

LPVOID KERNELAPI AllocateIoMemory(LPVOID PhysicalAddress, UINT64 NumPages, UINT Flags) {
	if(!NumPages) return NULL;
	// NumPages + 1 (Endof I/O Pool Marker to set a page fault when accessed)
	LPVOID Mem = AllocatePoolEx(&IoSpaceMemoryThread, (NumPages ) << 12, 0, ALLOCATE_POOL_PHYSICAL | ALLOCATE_POOL_NOT_RAM);
	if(!Mem) return NULL;
	UINT MapFlags = PM_MAP | PM_NX;
	if(Flags & IO_MEMORY_CACHE_DISABLED) MapFlags |= PM_CACHE_DISABLE;
	if(Flags & IO_MEMORY_WRITE_THROUGH) MapFlags |= PM_WRITE_THROUGH;
	if(Flags & IO_MEMORY_READ_ONLY) MapFlags |= PM_READWRITE;
	MapPhysicalPages(kproc->PageMap, Mem, PhysicalAddress, NumPages, MapFlags);
	// Set Endof I/O Pool Marker
	// MapPhysicalPages(kproc->PageMap, (char*)Mem + (NumPages << 12), NULL, 1, 0);
	return Mem;
}


BOOL KERNELAPI FreeIoMemory(LPVOID IoMem) {
	if(!IoMem) return FALSE;

	ALLOCATED_MEMORY_LIST* List = IoSpaceMemoryProcess.MemoryManagementTable.AllocatedMemory;
	ALLOCATED_MEMORY_LIST* LastList = List;
	for (;;) {
		if (!List->HeapCount) {
			// Unlink List And Lower memory usage
			if (LastList != List) {
				LastList->Next = List->Next;
				kfree((LPVOID)List);
				List = LastList;
			}
			goto NextList;
		}
		for (int i = List->SearchMax; i >= 0; i--) {
			if (List->Heaps[i].Address == (UINTPTR)IoMem && List->Heaps[i].Present) {
				PMEMORY_HEAP_DESCRIPTOR Descriptor = &List->Heaps[i];

				PFREE_HEAP_DESCRIPTOR FreeHeapDesc = AllocateFreeHeapDescriptor(
					&IoSpaceMemoryProcess, NULL, (LPVOID)Descriptor->Address, Descriptor->HeapLength
				);
				if (!FreeHeapDesc) SET_SOD_MEMORY_MANAGEMENT;
				
				IoSpaceMemoryProcess.MemoryManagementTable.PhysicalUsedMemory -= Descriptor->HeapLength;
				PhysicalMemoryStatus.AllocatedMemory -= Descriptor->HeapLength;
				if (!(Descriptor->Present & 2)) // Heap is not private
				{
					IoSpaceMemoryProcess.MemoryManagementTable.UsedMemory -= Descriptor->HeapLength;
				}

				LPVOID ReturnAddress = (LPVOID)Descriptor->Address;
				Descriptor->ParentList->HeapCount--;
				if (Descriptor->ParentList->SearchMax + 1 == Descriptor->Index) Descriptor->ParentList->SearchMax--;

				UINT64 NumPages = Descriptor->HeapLength >> 12;
				SystemDebugPrint(L"I/O Mem Free : Address : %x, NumPages : %x", ReturnAddress, NumPages);
				MapPhysicalPages(kproc->PageMap, ReturnAddress, NULL, NumPages, 0);
				Descriptor->Present = FALSE;
				return TRUE;
			}
		}
	NextList:
		if (!List->Next) break;
		List = List->Next;
		LastList = List;
	}
	return FALSE;
}