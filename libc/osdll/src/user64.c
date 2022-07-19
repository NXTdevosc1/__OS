#define __DLLEXPORTS
#include <user64.h>

_DECL void __cdecl SysDebugPrint(LPCSTR text){
    __SyscallNumber(SYSCALL_DEBUG_PRINT);
    EnterSystem((UINT64)text);
}
_DECL HPROCESS __cdecl GetCurrentProcess(){
    __SyscallNumber(SYSCALL_CURRENT_PROCESS);
    return (HPROCESS)EnterSystem(0);
}
_DECL HTHREAD __cdecl GetCurrentThread(){
    __SyscallNumber(SYSCALL_CURRENT_THREAD);
    return (HTHREAD)EnterSystem(0);
}

_DECL UINT64 __cdecl GetCurrentProcessId(){
    __SyscallNumber(SYSCALL_CURRENTPID);
    return (UINT64)EnterSystem(0);
}
_DECL UINT64 __cdecl GetCurrentThreadId(){
    __SyscallNumber(SYSCALL_CURRENT_THREADID);
    return (UINT64)EnterSystem(0);
}