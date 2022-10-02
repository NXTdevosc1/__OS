#pragma once
#include <krnltypes.h>
#include <CPU/descriptor_tables.h>
#define MAX_SYSCALL 20
#define SYSENTRY_STACK_NUMPAGES 0x30
#define SYSENTRY_STACK_BASE (CPU_BUFFER_IST3_BASE + (TSS_IST3_NUMPAGES << 12))

void MSABI SyscallDebugPrint(const char* data);

// Init SYSCALL/SYSENTER Instructions
void InitSysEntry(void* CpuBuffer);
void GlobalSysEntryTableInitialize(void);

extern void* GlobalSyscallTable[MAX_SYSCALL + 1];
extern long long __FastSysEntry(void);
extern long long _SyscallEntry(void);
extern UINT64 _syscall_max;
