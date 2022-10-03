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

    // Setup Syscall
    UINT64 Msr = __readmsr(0xC0000080);
    __writemsr(0xC0000080, Msr | 1); // Enable Syscall Extensions
    __writemsr(0xC0000081, (UINT64)(CS_KERNEL | (USER_SEGMENT_BASE << 16)) << 32); // set STAR msr

    // set syscall RIP

    __writemsr(0xC0000082, (UINT64)_SyscallEntry);

    // Setup Sysenter

    // Write IA32_SYSENTER_CS
    // SS = CS + 8, USER_CS = CS + 32, USER_SS = CS + 40


    __writemsr(0x174, CS_KERNEL);
    // Write IA32_SYSENTER_RSP
    UINT64 SyscallStack = (UINT64)CpuBuffer + SYSENTRY_STACK_BASE + 0xC000;

    __writemsr(0x175, SyscallStack);

// TODO :
    // Write IA32_SYSENTER_RIP
    // eax = (UINT32)((UINT64)__FastSysEntry);
    // edx = (UINT32)((UINT64)__FastSysEntry >> 32);

}

void GlobalSysEntryTableInitialize(){
    
    _syscall_max = MAX_SYSCALL;
    
    GlobalUserSystemConfig.PreferredSysenterMethod = 0; // SYSENTER = 0, SYSCALL = 8

}

UINT __dbg_y = 0;

void MSABI SyscallDebugPrint(const char* data){
    data = KeResolvePhysicalAddress(KeGetCurrentProcess(), data);
    if(!data) return;
    GP_draw_sf_text(data, 0xfffff, 20, 20 + __dbg_y * 20);
    __dbg_y++;
}