#include <MemoryManagement.h>
#include <CPU/process.h>

void* AllocatePoolEx(RFPROCESS Process, UINT64 NumBytes, UINT Align, UINT64 Flags) {
    if(!NumBytes || !Process) return NULL;
    if(NumBytes & 0xF) {
        NumBytes += 0x10;
        NumBytes &= ~0xF;
    }
    
    if(Process->MemoryManagementTable.AvailableMemory < NumBytes) {
        UINT64 Pages = NumBytes >> 12;
        if(NumBytes & 0xFFF) Pages++;


    }

    if(!(NumBytes & ~(0x3FF))) {
        // Below 1KB (fetch all lists)
        SystemDebugPrint(L"Below 1KB");
        for(UINT i = 0;i<NUM_FREE_MEMORY_LEVELS;i++) {
            
        }
    } else if(!(NumBytes & ~(0xFFFF))) {
        // Below 64 KB (fetch all lists from 64 kb)
        SystemDebugPrint(L"Below 64KB");
    } else if(!(NumBytes & ~(0x1FFFFF))) {
        // Below 2MB (fetch below and above 2MB lists)
        SystemDebugPrint(L"Below 2MB");
    } else {
        // Above (or Equal) 2MB
        SystemDebugPrint(L"Above 2MB");
    }
    return NULL;
}
void* RemoteFreePool(RFPROCESS Process, void* HeapAddress) {
    return NULL;
}

void* AllocatePool(UINT64 NumBytes) {
    return AllocatePoolEx(KeGetCurrentProcess(), NumBytes, 0, 0);
}
void* FreePool(void* HeapAddress) {
    return RemoteFreePool(KeGetCurrentProcess(), HeapAddress);
}