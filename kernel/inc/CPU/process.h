#pragma once
#include <CPU/descriptor_tables.h>
#include <CPU/paging.h>
#include <stdint.h>
#include <interrupt_manager/interrupts.h>
#include <CPU/process_defs.h>
#include <Management/runtimesymbols.h>
// performance states




UINT64 GlobalThreadPreemptionPriorities[];


typedef void (__cdecl *THREAD_START_ROUTINE)(void);

#define __STACK_PUSH(Stack, Var) Stack = (UINT64)Stack - 8; *(UINT64*)(Stack) = (UINT64)Var
#define KERNELRUNTIMEDLLW L"kernelruntime.dll"
#define KERNELRUNTIMEDLL "kernelruntime.dll"


RFPROCESS KEXPORT KERNELAPI KeCreateProcess(RFPROCESS ParentProcess, LPWSTR ProcessName, UINT64 Subsystem, UINT16 OperatingMode);
RFPROCESS KERNELAPI GetProcessById(UINT64 ProcessId);

BOOL KERNELAPI SetProcessName(RFPROCESS Process, LPWSTR ProcessName);


RFTHREAD KEXPORT KERNELAPI KeCreateThread(RFPROCESS Process, UINT64 StackSize, THREAD_START_ROUTINE StartAddress, UINT64 Flags, UINT64* ThreadId); // parameters
KERNELSTATUS KERNELAPI SetThreadPriority(RFTHREAD Thread, int Priority);
KERNELSTATUS KERNELAPI SetPriorityClass(RFPROCESS Process, int PriorityClass);


int KERNELAPI TerminateCurrentProcess(int ExitCode);
int KERNELAPI TerminateCurrentThread(int ExitCode);
int KERNELAPI TerminateProcess(RFPROCESS process, int ExitCode);
int KERNELAPI TerminateThread(RFTHREAD thread, int ExitCode);

RFPROCESS KEXPORT KERNELAPI KeGetCurrentProcess(void);
RFTHREAD KEXPORT KERNELAPI KeGetCurrentThread(void);
RFTHREAD KEXPORT KERNELAPI KeGetProcessorIdleThread(UINT64 ProcessorId);

UINT64 KEXPORT KERNELAPI KeGetCurrentProcessId(void);
UINT64 KEXPORT KERNELAPI KeGetCurrentThreadId(void);

BOOL KEXPORT KERNELAPI KeSuspendThread(RFTHREAD Thread);
BOOL KEXPORT KERNELAPI KeResumeThread(RFTHREAD Thread);

extern PROCESSMGRTABLE Pmgrt;
extern void ScheduleTask(void);
extern void SkipTaskSchedule(void);


double KERNELAPI GetThreadCpuTime(RFTHREAD Thread);
double KERNELAPI GetIdleCpuTime(void);
double KERNELAPI GetTotalCpuTime(void);
double KERNELAPI GetProcessorIdleTime(UINT64 ProcessorId);
double KERNELAPI GetProcessorTotalTime(UINT64 ProcessorId);

BOOL SetThreadProcessor(RFTHREAD Thread, UINT ProcessorId);


void KERNELAPI TaskSchedulerEnable(void);
void KERNELAPI TaskSchedulerDisable(void);

BOOL KEXPORT KERNELAPI IoWait(void);
BOOL KEXPORT KERNELAPI IoFinish(RFTHREAD Thread);

BOOL isValidProcess(RFPROCESS Process);