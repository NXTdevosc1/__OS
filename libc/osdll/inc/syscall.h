#pragma once
enum SYSCALL_TABLE{
    SYSCALL_SYSIDENTIFY = 0,
    SYSCALL_DEBUG_PRINT = 1,
    SYSCALL_CURRENTPID = 2,
    SYSCALL_CURRENT_THREADID = 3,
    SYSCALL_MALLOC = 4,
    SYSCALL_EXTENDED_ALLOC = 5,
    SYSCALL_FREE = 6,
    SYSCALL_MEMSTAT = 7,
    SYSCALL_TERMINATE_CURRENT_PROCESS = 8,
    SYSCALL_TERMINATE_CURRENT_THREAD = 9,
    SYSCALL_TERMINATE_PROCESS = 10,
    SYSCALL_TERMINATE_THREAD = 11,
    SYSCALL_TOTAL_CPU_TIME = 12,
    SYSCALL_THREAD_CPU_TIME = 13,
    SYSCALL_IDLE_CPU_TIME = 14,
    SYSCALL_CURRENT_PROCESS = 15,
    SYSCALL_CURRENT_THREAD = 16,

};
typedef unsigned long long _SCA; // Syscall Argument

extern unsigned long long __cdecl EnterSystem(_SCA FirstArg, ...);
extern unsigned long long __stdcall __stdcall__EnterSystem(_SCA FirstArg, ...);

extern void __cdecl __SyscallNumber(unsigned long long SyscallNumber);
//extern void __SyscallArg(unsigned long long Arg);
