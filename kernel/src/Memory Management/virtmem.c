#include <CPU/process_defs.h>
#include <MemoryManagement.h>
#include <intrin.h>
#include <CPU/paging.h>
#include <kernel.h>
// Todo : implement an internal process lock
void KEXPORT *KERNELAPI KeAllocateVirtualMemory(RFPROCESS Process, UINT64 NumBytes) {
    UINT64 NumPages = NumBytes >> 12;
    if(NumBytes & 0x1FF) NumPages++;
    RFPAGEMAP PageMap = Process->PageMap;

    UINT Pages = 0;


    
    return VirtualFindAvailableMemory(kproc->PageMap, (LPVOID)0x1000, (LPVOID)0xF0000000000, 0x2000);
}