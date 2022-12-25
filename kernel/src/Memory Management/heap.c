#include <MemoryManagement.h>
#include <CPU/process.h>
#include <interrupt_manager/SOD.h>
#include <intrin.h>
#include <CPU/cpu.h>

FREE_MEMORY_SEGMENT* AllocateFreeMemorySegment(FREE_MEMORY_TREE* Tree);

void* AllocatePoolEx(RFPROCESS Process, UINT64 NumBytes, UINT Align, UINT64 Flags) {
    if(!NumBytes || !Process) return NULL;
    if(NumBytes & 0xF) {
        NumBytes += 0x10;
        NumBytes &= ~0xF;
    }
    for(;;) {
        for(UINT i = 0;i<0x100;i++) _mm_pause();

        AllocateFreeMemorySegment(&Process->MemoryManagementTable.FreeMemory);
    }
    
    // Check reserved, not allocated memory
    if(Process->MemoryManagementTable.ReservedPages < NumBytes) {
        UINT64 Pages = NumBytes >> 12;
        if(NumBytes & 0xFFF) Pages++;


    }

    if(!(NumBytes & ~(0x3FF))) {
        // Below 1KB (fetch all lists)
        // SystemDebugPrint(L"Below 1KB");
        for(UINT i = 0;i<NUM_FREE_MEMORY_LEVELS;i++) {
            
        }
    } else if(!(NumBytes & ~(0xFFFF))) {
        // Below 64 KB (fetch all lists from 64 kb)
        // SystemDebugPrint(L"Below 64KB");
    } else if(!(NumBytes & ~(0x1FFFFF))) {
        // Below 2MB (fetch below and above 2MB lists)
        // SystemDebugPrint(L"Below 2MB");
    } else {
        // Above (or Equal) 2MB
        // SystemDebugPrint(L"Above 2MB");
    }
    return NULL;
}
void* RemoteFreePool(RFPROCESS Process, void* HeapAddress) {
    return NULL;
}

void* AllocatePool(UINT64 NumBytes) {
    return AllocatePoolEx(kproc, NumBytes, 0, 0);
}
void* FreePool(void* HeapAddress) {
    return RemoteFreePool(KeGetCurrentProcess(), HeapAddress);
}

RFMEMORY_SEGMENT MemMgr_CreateInitialHeap(void* HeapAddress, UINT64 HeapLength) {
    return NULL;
}

#define _spalloc() _SIMD_AllocatePhysicalPage(MemoryManagementTable.PageBitmap, MemoryManagementTable.NumBytesPageBitmap, MemoryManagementTable.PageArray)

static inline BOOL MmAllocateSlot(FREE_MEMORY_TREE* Tree, ULONG* _Indx) {
    ULONG Index;
    for(ULONG x = 0;x<3;x++) {
        UINT64 p = ~Tree->Present[x];
        while(_BitScanForward64(&Index, p)) {
            _bittestandreset64(&p, Index);
            ULONG BiIndx = Index;
            Index += (x << 6);
            if(!_interlockedbittestandset(&Tree->Childs[Index].LockMutex, 0)) {
                Tree->Childs[Index].FullSlotsCount = 0;
                Tree->Childs[Index].LargestHeap = 0;
                Tree->Childs[Index].Child = _spalloc();
                if(!Tree->Childs[Index].Child) SET_SOD_MEMORY_MANAGEMENT;
                ZeroMemory(Tree->Childs[Index].Child, 0x1000);
                _bittestandset64((long long*)&Tree->Present[x], BiIndx);
                Tree->PresentSlotCount++;
                *_Indx = Index;

                return TRUE;
            }
        }
    }
    return FALSE;
}

static inline ULONG FindNextAvailableChild(FREE_MEMORY_TREE* Parent, BOOL _Root) {
    // Keep looping on the root tree

    ULONG Index;
        
    for(;;) {
        if(Parent->FullSlotCount == MEMTREE_MAX_SLOTS) return (ULONG)-1;
        if(Parent->PresentSlotCount == Parent->FullSlotCount) goto Allocate;
        for(UINT b = 0;b<3;b++) {
            UINT64 f0 = (~Parent->FullSlots[b]) & Parent->Present[b];
            while(_BitScanForward64(&Index, f0)) {
                // Found a tree with free slots
                _bittestandreset64(&f0, Index);
                ULONG i = Index + (b << 6);
                if(_Root && _interlockedbittestandset(&Parent->Childs[i].LockMutex, 0)) {
                    continue;
                }
                if(Parent->Childs[i].FullSlotsCount == MEMTREE_MAX_SLOTS) {
                    _interlockedbittestandreset(&Parent->Childs[i].LockMutex, 0);
                    continue;
                }
                return i;
            }
        }
        Allocate:
        if(!MmAllocateSlot(Parent, &Index)) return (ULONG)-1;
        else return Index;
    }



    return Index;
}

// Allocates a segment descriptor for the heap
FREE_MEMORY_SEGMENT* AllocateFreeMemorySegment(FREE_MEMORY_TREE* Tree) {
    FREE_MEMORY_TREE* Current;
// Retry:
    Current = Tree;
    for(;;) {
        while(Current->FullSlotCount != MEMTREE_MAX_SLOTS) {
            ULONG idx;
            idx = FindNextAvailableChild(Current, TRUE);
            if(idx == (ULONG)-1) goto NextTree;
            FREE_MEMORY_TREE* Lvl1 = Current->Childs[idx].Child;
            ULONG idx1 = FindNextAvailableChild(Lvl1, FALSE);
            if(idx1 == (ULONG)-1) SET_SOD_MEMORY_MANAGEMENT; // There is a software bug

            FREE_MEMORY_TREE* Lvl2 = Lvl1->Childs[idx1].Child;
            ULONG idx2 = FindNextAvailableChild(Lvl2, FALSE);
            if(idx2 == (ULONG)-1) SET_SOD_MEMORY_MANAGEMENT;
            FREE_MEMORY_TREE* Lvl3 = Lvl2->Childs[idx2].Child;
            ULONG idx3 = FindNextAvailableChild(Lvl3, FALSE);
            if(idx3 == (ULONG)-1) SET_SOD_MEMORY_MANAGEMENT;
            FREE_MEMORY_SEGMENT_LIST_HEAD* ListHead = Lvl3->Childs[idx3].Child;
            ULONG idx4 = (ULONG)-1;

            for(UINT b = 0;b<3;b++) {
                UINT64 FreeSlots = ~ListHead->UsedSlots[b];
                if(_BitScanForward64(&idx4, FreeSlots)) {
                    _bittestandset64((long long*)ListHead->UsedSlots + b, idx4);
                    idx4 = idx4 + (b << 6);
                    break;
                }
            }
            if(idx4 == (ULONG)-1) SET_SOD_MEMORY_MANAGEMENT;
            ListHead->NumUsedSlots++;

            // Track full tree slots
            if(ListHead->NumUsedSlots == MEMTREE_MAX_SLOTS) {
                SystemDebugPrint(L"LH_FULL_SLOTS");
                Lvl3->FullSlotCount++;
                _bittestandset64(&Lvl3->FullSlots[idx3 >> 6], (idx3 & 0x3F));
                if(Lvl3->FullSlotCount == MEMTREE_MAX_SLOTS) {
                    SystemDebugPrint(L"LVL3_FULL_SLOTS");

                    Lvl2->FullSlotCount++;
                    _bittestandset64(&Lvl2->FullSlots[idx2 >> 6], (idx2 & 0x3F));
                    if(Lvl2->FullSlotCount == MEMTREE_MAX_SLOTS) {
                    SystemDebugPrint(L"LVL2_FULL_SLOTS");

                        Lvl1->FullSlotCount++;
                        _bittestandset64(&Lvl1->FullSlots[idx1 >> 6], (idx1 & 0x3F));
                        if(Lvl1->FullSlotCount == MEMTREE_MAX_SLOTS) {
                SystemDebugPrint(L"LVL1_FULL_SLOTS");

                            Current->FullSlotCount++;
                            _bittestandset64(&Current->FullSlots[idx >> 6], (idx & 0x3F));
                        }
                    }
                }
            }
            // Release Tree
            _interlockedbittestandreset(&Current->Childs[idx].LockMutex, 0);
            
            return &ListHead->MemorySegments[idx4];
        }
NextTree:
        if(!Current->Next) {
            SystemDebugPrint(L"NEW_TREE");
            Current->Next =  _spalloc();
            if(!Current->Next) SET_SOD_LESS_MEMORY;
            ZeroMemory(Current->Next, sizeof(FREE_MEMORY_TREE));
        }
        Current = Current->Next;
    }
}