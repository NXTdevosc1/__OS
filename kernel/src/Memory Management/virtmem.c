#include <CPU/process_defs.h>
#include <MemoryManagement.h>
#include <intrin.h>
#include <CPU/paging.h>
#include <kernel.h>
#include <CPU/cpu.h>
#include <math.h>
#include <interrupt_manager/SOD.h>
// Allocates in kernel mode RAM Space
void KEXPORT *KERNELAPI KeAllocateVirtualMemory(UINT64 NumBytes) {
    UINT64 NumPages = NumBytes >> 12;
    if(NumBytes & 0xFFF) NumPages++;
    MutexWait64(kproc->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_VIRTUAL_MEMORY);
    LPVOID Vmem = VirtualFindAvailableMemory(kproc, (LPVOID)((char*)SystemSpaceBase + SYSTEM_SPACE48_RAM), (LPVOID)((char*)SystemSpaceBase + SYSTEM_SPACE48_MAX), NumPages);
    if(!Vmem) {
        MutexRelease64(kproc->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_VIRTUAL_MEMORY);
    }
    
    if(!KeAllocateFragmentedPages(kproc, Vmem, NumPages)) {
        MutexRelease64(kproc->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_VIRTUAL_MEMORY);
        SET_SOD_MEMORY_MANAGEMENT;
    }
    MutexRelease64(kproc->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_VIRTUAL_MEMORY);
    return Vmem;
}
// Allocates in user mode space (Random area inside the low 128 TB)
