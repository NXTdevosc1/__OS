#include <Management/handle.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <fs/fs.h>
#include <kernel.h>
#define CalculateId(IteratorId, ListIndex, HandleIndex) (IteratorId * HANDLE_COUNT_PER_LIST * HANDLE_COUNT_PER_LIST + ListIndex * HANDLE_COUNT_PER_LIST + HandleIndex)


HANDLE_TABLE* CreateHandleTable() {

	HANDLE_TABLE* tbl = AllocatePool(sizeof(HANDLE_TABLE));
	if (!tbl) SET_SOD_MEMORY_MANAGEMENT;
	SZeroMemory(tbl);
	tbl->HandleIterator.Lists[0] = AllocatePool(sizeof(HANDLE_LIST));
	if (!tbl->HandleIterator.Lists[0]) SET_SOD_MEMORY_MANAGEMENT;
	SZeroMemory(tbl->HandleIterator.Lists[0]);
	OpenHandle(tbl, NULL, HANDLE_FLAG_FREE_ON_EXIT, HANDLE_KERNEL_ALLOCATE, tbl, NULL); // Create a handle to free this list
	OpenHandle(tbl, NULL, HANDLE_FLAG_FREE_ON_EXIT, HANDLE_KERNEL_ALLOCATE, tbl->HandleIterator.Lists[0], NULL); // Create a handle to free this list

	return tbl;
}

HANDLE GetHandle(HANDLE_TABLE* HandleTable, const void* Data) {
	if (!HandleTable || !Data) return NULL;
	PHANDLE_ITERATOR Iterator = &HandleTable->HandleIterator;
	
	for (;;) {
		for (UINT64 a = 0; a < HANDLE_COUNT_PER_LIST; a++) {
			if (Iterator->Lists[a] && Iterator->Lists[a]->HandleCount) {
				PHANDLE_LIST list = Iterator->Lists[a];
				for (UINT64 i = 0; i < HANDLE_COUNT_PER_LIST; i++) {
					if (list->Handles[i].Data == Data && (list->Handles[i].Flags & HANDLE_FLAG_PRESENT)) {
						return &list->Handles[i];
					}
				}
			}
		}
		if (!Iterator->Next) break;
		Iterator = Iterator->Next;
	};
	return NULL;
}

HANDLE OpenHandle(HANDLE_TABLE* HandleTable, RFTHREAD Thread, UINT64 Flags, UINT64 DataType, void* Data, HANDLE_RELEASE_PROCEDURE ReleaseProcedure) {
	if (!Data) return NULL;
	PHANDLE_ITERATOR Iterator = &HandleTable->HandleIterator;
	UINT64 IteratorId = 0;
	// Search for free Iterator
	for(;;){
		for (UINT64 i = 0; i < HANDLE_COUNT_PER_LIST; i++) {
			if (Iterator->Lists[i] && Iterator->Lists[i]->HandleCount < HANDLE_COUNT_PER_LIST) {
				PHANDLE_LIST list = Iterator->Lists[i];
				for (UINT64 x = 0; x < HANDLE_COUNT_PER_LIST; x++) {
					if (!(list->Handles[x].Flags & HANDLE_FLAG_PRESENT)) {
						HANDLE Handle = &list->Handles[x];
						Handle->Thread = Thread;
						Handle->DataType = DataType;
						Handle->Flags = Flags | HANDLE_FLAG_PRESENT;
						Handle->HandleId = CalculateId(IteratorId, i, x);
						Handle->Data = Data;
						Handle->ReleaseProcedure = ReleaseProcedure;
						list->HandleCount++;
						return Handle;
					}
				}
			}
			else if (!Iterator->Lists[i]) {
				Iterator->Lists[i] = AllocatePool(sizeof(HANDLE_LIST));
				if (!Iterator->Lists[i]) SET_SOD_MEMORY_MANAGEMENT;
				SZeroMemory(Iterator->Lists[i]);
				OpenHandle(HandleTable, Thread, HANDLE_FLAG_FREE_ON_EXIT, HANDLE_KERNEL_ALLOCATE, Iterator->Lists[i], NULL); // Create a handle to free this list
				return OpenHandle(HandleTable, Thread, Flags, DataType, Data, ReleaseProcedure);
			}
		}
		if (!Iterator->Next) {
			Iterator->Next = AllocatePool(sizeof(HANDLE_ITERATOR));
			if (!Iterator->Next) SET_SOD_MEMORY_MANAGEMENT;
			SZeroMemory(Iterator->Next);
			Iterator->Lists[0] = AllocatePool(sizeof(HANDLE_LIST));
			if (!Iterator->Lists[0]) SET_SOD_MEMORY_MANAGEMENT;
			SZeroMemory(Iterator->Lists[0]);
			OpenHandle(HandleTable, Thread, HANDLE_FLAG_FREE_ON_EXIT, HANDLE_KERNEL_ALLOCATE, Iterator->Next, NULL); // Create a handle to free this list

			return OpenHandle(HandleTable, Thread, Flags, DataType, Data, ReleaseProcedure);
		}
		Iterator = Iterator->Next;
		IteratorId++;
	}

	return NULL;
}

HRESULT CloseHandle(HANDLE Handle) {
	if (!Handle) return -1;
	if (!(Handle->Flags & HANDLE_FLAG_PRESENT)) return FALSE;
	if (Handle->Flags & HANDLE_FLAG_FREE_ON_EXIT) {
		RemoteFreePool(kproc, Handle->Data);
	}
	if (Handle->Flags & HANDLE_FLAG_CLOSE_FILE_ON_EXIT) {
		CloseFile(Handle->Data);
	}
	HRESULT Result = SUCCESS;
	if (Handle->ReleaseProcedure) {
		Result = Handle->ReleaseProcedure(Handle);
		if (Result == 0) {
			Handle->Flags &= ~(HANDLE_FLAG_PRESENT);
		}
	}
	Handle->Flags = 0;
	Handle->HandleList->HandleCount--;
	return Result;
}

BOOL AcquireHandle(HANDLE Handle) {
	return TRUE;
}

BOOL StartHandleIteration(HANDLE_TABLE* HandleTable, PHANDLE_ITERATION_STRUCTURE IterationStructure) {
	if (IterationStructure->Set || !HandleTable || !IterationStructure) return FALSE;
	SZeroMemory(IterationStructure);
	IterationStructure->HandleTable = HandleTable;
	IterationStructure->HandleIterator = &HandleTable->HandleIterator;
	IterationStructure->Set = TRUE;
	IterationStructure->Usable = TRUE;

	return TRUE;
}
HANDLE GetNextHandle(PHANDLE_ITERATION_STRUCTURE Iteration) {
	if (!Iteration->Set || !Iteration->Usable) return NULL;
	if (Iteration->ListIndex == HANDLE_COUNT_PER_LIST) {
		Iteration->ListIndex = 0;
		Iteration->HandleIndex = 0;
		if (!Iteration->HandleIterator->Next) Iteration->Usable = FALSE;
		else Iteration->HandleIterator = Iteration->HandleIterator->Next;
	}
	if (Iteration->HandleIndex == HANDLE_COUNT_PER_LIST) {
		Iteration->ListIndex++;
		Iteration->HandleIndex = 0;
	}

	PHANDLE_ITERATOR Iterator = Iteration->HandleIterator;
	if (!Iterator) return NULL;

	if (!Iterator->Lists[Iteration->ListIndex]) return NULL;
	PHANDLE_LIST List = Iterator->Lists[Iteration->ListIndex];
	if (!(List->Handles[Iteration->HandleIndex].Flags & HANDLE_FLAG_PRESENT)) return NULL;
	HANDLE Handle = &List->Handles[Iteration->HandleIndex];
	Iteration->HandleIndex++;
	return Handle;
}
BOOL EndHandleIteration(PHANDLE_ITERATION_STRUCTURE IterationStructure) {
	if (!IterationStructure) return FALSE;
	SZeroMemory(IterationStructure);
	return TRUE;
}