#include <MemoryManagement.h>
#include <CPU/process.h>
#include <interrupt_manager/SOD.h>
#include <intrin.h>
#include <CPU/cpu.h>

FREE_MEMORY_SEGMENT* KERNELAPI CreateHeap(FREE_MEMORY_TREE* Tree, void* HeapAddress, UINT64 HeapLength);


void* AllocatePoolEx(RFPROCESS Process, UINT64 NumBytes, UINT Align, UINT64 Flags) {
    if(!NumBytes || !Process) return NULL;
    if(NumBytes & 0xF) {
        NumBytes += 0x10;
        NumBytes &= ~0xF;
    }
    FREE_MEMORY_TREE* Current = &Process->MemoryManagementTable.FreeMemory;
    ULONG Index;
    UINT64 Bmp;
    // Step 1 : Find a free block (TODO After multiprocessing : CPU Sync)
    for(;;) {
        if(Current->FullSlotCount == MEMTREE_MAX_SLOTS) goto continue0;
        for(UINT x0 = 0;x0<MEMTREE_NUM_SLOT_GROUPS;x0) {
            Bmp = Current->FullSlots[x0];
            while(_BitScanForward64(&Index, Bmp)) {
                _bittestandreset64(&Bmp, Index);
                Index += (x0 << 7);
                if(Current->Childs[Index].Child && Current->Childs[Index].LargestHeap->HeapLength >= NumBytes) {
                    // We found a heap
                    FREE_MEMORY_SEGMENT* Heap = Current->Childs[Index].LargestHeap;
                    void* Address = Heap->Address;
                    (char*)Heap->Address += NumBytes;
                    Heap->HeapLength -= NumBytes;
                    
                    FREE_MEMORY_SEGMENT_LIST_HEAD* LhLargestHeap = Current->Childs[Index].ListHeadLargestHeap;
                    
                    if(!Heap->HeapLength && !Heap->InitialHeap) {
                        UINT ChainId = Current->Childs[Index].IndexLargestHeap >> 7 /* / MEMTREE_MAX_SLOTS*/;
                        // Free heap slot
                        _bittestandreset64(&LhLargestHeap->UsedSlots[ChainId], Current->Childs[Index].IndexLargestHeap & (MEMTREE_MAX_SLOTS - 1));
                        LhLargestHeap->NumUsedSlots--;
                        
                        // Clear full slot count
                        ChainId = LhLargestHeap->ParentItemIndex >> 7;
                        FREE_MEMORY_TREE* Tree = LhLargestHeap->Parent; // LVL2
                        if(_bittestandreset64(&Tree->FullSlots[ChainId], LhLargestHeap->ParentItemIndex & (MEMTREE_MAX_SLOTS - 1))) {
                            Tree->FullSlotCount--;
                            ChainId = Tree->ParentItemIndex >> 7;
                            if(_bittestandreset64(&Tree->Parent->FullSlots[ChainId], Tree->ParentItemIndex & (MEMTREE_MAX_SLOTS - 1))) {
                                Tree->Parent->FullSlotCount--;
                            }
                        } 
                    }
                    // Now determine the biggest heap
                    
                }
            }

        }
        continue0:
        if(!Current->Next) break;
        Current = Current->Next;
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

RFMEMORY_SEGMENT KERNELAPI MemMgr_CreateInitialHeap(RFPROCESS Process, void* HeapAddress, UINT64 HeapLength) {
    // Search for free memory ending 
    return NULL;
}

#define _spalloc() _SIMD_AllocatePhysicalPage()

#define _mm256_treeclr(_Tree) __stosq((unsigned long long*)_Tree, 0, 512)


static inline BOOL KERNELAPI MmAllocateSlot(FREE_MEMORY_TREE* Tree, ULONG* _Indx) {
    ULONG Index;
    for(ULONG x = 0;x<MEMTREE_NUM_SLOT_GROUPS;x++) {
        UINT64 p = ~Tree->Present[x];
        while(_BitScanForward64(&Index, p)) {
            _bittestandreset64(&p, Index);
            ULONG BiIndx = Index;
            Index += (x << 6);
            if(Tree->Root && _interlockedbittestandset(&Tree->Childs[Index].LockMutex, 0)) continue;

            Tree->Childs[Index].Child = _spalloc();
            if(!Tree->Childs[Index].Child) SET_SOD_MEMORY_MANAGEMENT;
            _mm256_treeclr(Tree->Childs[Index].Child);
            _bittestandset64((long long*)&Tree->Present[x], BiIndx);
            Tree->PresentSlotCount++;
            *_Indx = Index;

            return TRUE;
        }
    }
    return FALSE;
}

// Optimization note : SecondLargestHeap is used for : 
// if HeapLength >= SecondLargestHeap && Tree.NumFullSlots != MAX_SLOTS :
// Find a slot where to set LargestHeap
// if not found, then set SecondLargestHeap in the new slot

static inline ULONG KERNELAPI FindNextAvailableChild(FREE_MEMORY_TREE* Parent) {
    // Keep looping on the root tree

    ULONG Index;
        
    for(;;) {
        if(Parent->PresentSlotCount == Parent->FullSlotCount) goto Allocate;
        for(UINT b = 0;b<MEMTREE_NUM_SLOT_GROUPS;b++) {
            UINT64 f0 = (~Parent->FullSlots[b]) & Parent->Present[b];
            while(_BitScanForward64(&Index, f0)) {
                // Found a tree with free slots
                _bittestandreset64(&f0, Index);
                ULONG i = Index + (b << 6);
                if(Parent->Root && _interlockedbittestandset(&Parent->Childs[i].LockMutex, 0)) {
                    continue;
                }
                
                if(*(UINT8*)Parent->Childs[i].Child == MEMTREE_MAX_SLOTS) {
                    if(Parent->Root)
                        _interlockedbittestandreset(&Parent->Childs[i].LockMutex, 0);
                    
                    continue;
                }
                return i;
            }
        }
        Allocate:
        if(!MmAllocateSlot(Parent, &Index)) {
            SystemDebugPrint(L"SF");
            return (ULONG)-1;
        }
        else return Index;
    }



    return Index;
}

// Initial heap is the heap with index 0

// Allocates a segment descriptor for the heap
FREE_MEMORY_SEGMENT* KERNELAPI CreateHeap(FREE_MEMORY_TREE* Tree, void* HeapAddress, UINT64 HeapLength) {
    FREE_MEMORY_TREE* Current;
    // if(Tree->InitialHeap && ((UINT64)Tree->InitialHeap->Address + Tree->InitialHeap->HeapLength) == (UINT64)HeapAddress) {
    //     Tree->InitialHeap->HeapLength += HeapLength;
    //     return Tree->InitialHeap;
    // }
// Retry:
    Current = Tree;
    for(;;) {
        while(Current->FullSlotCount != MEMTREE_MAX_SLOTS) {
            ULONG idx;
            idx = FindNextAvailableChild(Current);
            if(idx == (ULONG)-1) goto NextTree;
            FREE_MEMORY_TREE* Lvl1 = Current->Childs[idx].Child;
            ULONG idx1 = FindNextAvailableChild(Lvl1);
            if(idx1 == (ULONG)-1) SET_SOD_MEMORY_MANAGEMENT; // There is a software bug

            FREE_MEMORY_TREE* Lvl2 = Lvl1->Childs[idx1].Child;
            ULONG idx2 = FindNextAvailableChild(Lvl2);
            if(idx2 == (ULONG)-1) SET_SOD_MEMORY_MANAGEMENT;
            FREE_MEMORY_SEGMENT_LIST_HEAD* ListHead = Lvl2->Childs[idx2].Child;
            ULONG idxlh = (ULONG)-1;

            // Now allocate a segment for the heap

            for(UINT b = 0;b<MEMTREE_NUM_SLOT_GROUPS;b++) {
                UINT64 FreeSlots = ~ListHead->UsedSlots[b];
                if(_BitScanForward64(&idxlh, FreeSlots)) {
                    _bittestandset64((long long*)ListHead->UsedSlots + b, idxlh);
                    idxlh = idxlh + (b << 6);
                    break;
                }
            }
            if(idxlh == (ULONG)-1) SET_SOD_MEMORY_MANAGEMENT;
            ListHead->NumUsedSlots++;

            // Track full tree slots
            if(ListHead->NumUsedSlots == MEMTREE_MAX_SLOTS) {
                Lvl2->FullSlotCount++;
                _bittestandset64(&Lvl2->FullSlots[idx2 >> 6], (idx2 & 0x3F));
                if(Lvl2->FullSlotCount == MEMTREE_MAX_SLOTS) {
                    Lvl1->FullSlotCount++;
                    _bittestandset64(&Lvl1->FullSlots[idx1 >> 6], (idx1 & 0x3F));
                    if(Lvl1->FullSlotCount == MEMTREE_MAX_SLOTS) {
                        Current->FullSlotCount++;
                        _bittestandset64(&Current->FullSlots[idx1 >> 6], (idx1 & 0x3F));
                    }
                }
            }
            // Release Tree
            _interlockedbittestandreset(&Current->Childs[idx].LockMutex, 0);
            
            ListHead->MemorySegments[idxlh].Address = HeapAddress;
            ListHead->MemorySegments[idxlh].HeapLength = HeapLength;

            // Check largest heap tree
            if(!Lvl2->Childs[idx2].LargestHeap || Lvl2->Childs[idx2].LargestHeap->HeapLength < HeapLength) {
                Lvl2->Childs[idx2].LargestHeap = &ListHead->MemorySegments[idxlh];
                if(!Lvl2->Childs[idx2].LargestHeap || Lvl1->Childs[idx1].LargestHeap->HeapLength < HeapLength) {
                    Lvl1->Childs[idx1].LargestHeap = &ListHead->MemorySegments[idxlh];
                    if(!Current->Childs[idx].LargestHeap || Current->Childs[idx].LargestHeap->HeapLength < HeapLength) {
                        Current->Childs[idx].LargestHeap = &ListHead->MemorySegments[idxlh];
                    }
                }
            }

            return &ListHead->MemorySegments[idxlh];
        }
NextTree:
        if(!Current->Next) {
            SystemDebugPrint(L"NEW_TREE");
            while(1);
            Current->Next =  _spalloc();
            if(!Current->Next) SET_SOD_LESS_MEMORY;
            _mm256_treeclr(Current->Next);
        }
        Current = Current->Next;
    }
}