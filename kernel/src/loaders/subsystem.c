#include <loaders/subsystem.h>
#include <stddef.h>
#include <CPU/paging.h>

int ThreadWrapperInit(RFTHREAD Thread, void* EntryPoint){
    if(!Thread || !Thread->State) return -1;

    Thread->Registers.rax = (UINT64)EntryPoint;
    
    if (Thread->Process->OperatingMode == KERNELMODE_PROCESS) {
        Thread->Registers.rip = (UINT64)KernelThreadWrapper;
    }
    else {
        MapPhysicalPages(Thread->Process->PageMap, (LPVOID)GLOBAL_SUBSYSTEM_LOAD_ADDRESS, (LPVOID)UserThreadWrapper, 1, PM_PRESENT | PM_USER);

        Thread->Registers.rip = GLOBAL_SUBSYSTEM_LOAD_ADDRESS;
    }
    Thread->Registers.rip = (UINT64)Thread->Registers.rip;
    return SUCCESS;
}