#include <sysentry/sysentry.h>
#include <cstr.h>
#include <preos_renderer.h>
#include <MemoryManagement.h>
#include <CPU/process.h>
#include <CPU/descriptor_tables.h>
#include <sys/identify.h>
#include <CPU/paging.h>
#include <CPU/cpu.h>

void* GlobalSyscallTable[MAX_SYSCALL + 1] = {
    NULL
};

//void* __GlobalSyscallTable[] = {
//    SysIdentify, SyscallDebugPrint, GetCurrentProcessId, GetCurrentThreadId,
//    UserMalloc, UserExtendedMalloc, UserFree, GlobalMemoryStatus, TerminateCurrentProcess,
//    TerminateCurrentThread, TerminateProcess, TerminateThread
//};



void InitSysEntry(void* CpuBuffer) {
    uint32_t eax = 0, edx = 0;

    // Setup Syscall

    __ReadMsr(0xC0000080, &eax, &edx);
    __WriteMsr(0xC0000080, eax | 1, edx); // Enable Syscall Extensions
    eax = 0; // not set in 64 bit, Syscall EIP
    edx = CS_KERNEL | (USER_SEGMENT_BASE << 16); // kernel and user CS
    __WriteMsr(0xC0000081, eax, edx); // set STAR msr

    // set syscall RIP
    eax = (uint64_t)_SyscallEntry;
    edx = (uint64_t)_SyscallEntry >> 32;

    __WriteMsr(0xC0000082, eax, edx);

    // Setup Sysenter

    // Write IA32_SYSENTER_CS
    // SS = CS + 8, USER_CS = CS + 32, USER_SS = CS + 40


    eax = (UINT64)CS_KERNEL;
    edx = 0;

    __WriteMsr(0x174, eax, edx);
    // Write IA32_SYSENTER_RSP
    UINT64 SyscallStack = (UINT64)CpuBuffer + SYSENTRY_STACK_BASE + 0xC000;
    

    eax = SyscallStack;
    edx = SyscallStack >> 32;

    __WriteMsr(0x175, eax, edx);

    // Write IA32_SYSENTER_RIP
    eax = (UINT64)__FastSysEntry;
    edx = (UINT64)__FastSysEntry >> 32;

}

void GlobalSysEntryTableInitialize(){
    
    _syscall_max = MAX_SYSCALL;
    
    GlobalUserSystemConfig.PreferredSysenterMethod = 0; // SYSENTER = 0, SYSCALL = 8

}

UINT64 __dbg_y = 0;

void MSABI SyscallDebugPrint(const char* data){
    data = KeResolvePhysicalAddress(KeGetCurrentProcess(), data);
    if(!data) return;
    GP_draw_sf_text(data, 0xfffff, 20, 20 + __dbg_y * 20);
    __dbg_y++;
}