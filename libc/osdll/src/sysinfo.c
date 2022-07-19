#define __DLLEXPORTS

#include <sysinfo.h>

_DECL float GetThreadCpuTime(HTHREAD Thread){
    __SyscallNumber(SYSCALL_THREAD_CPU_TIME);
    return (float)EnterSystem((_SCA)Thread);
}
_DECL float GetProcessCpuTime(HPROCESS Process){
    return 0;
}
_DECL float GetTotalCpuTime(){
    __SyscallNumber(SYSCALL_TOTAL_CPU_TIME);
    return (float)EnterSystem(0);
}
_DECL float GetIdleCpuTime(){
    __SyscallNumber(SYSCALL_IDLE_CPU_TIME);
    return (float)EnterSystem(0);
}

_DECL UINT64 GetTotalCpuTimerClocks(){
    return 0;
}
_DECL UINT64 GetThreadCpuTimerClocks(){
    return 0;
}
_DECL UINT64 GetProcessCpuTimerClocks(){
    return 0;
}
_DECL UINT64 GetIdleCpuTimerClocks(){
    return 0;
}