#pragma once
#include <CPU/descriptor_tables.h>
#include <CPU/paging.h>
#include <stdint.h>
#include <interrupt_manager/interrupts.h>
#include <CPU/process_defs.h>
#include <Management/runtimesymbols.h>
// performance states




UINT64 GlobalThreadPreemptionPriorities[];


typedef void (__cdecl *THREAD_START_ROUTINE)();

#define __STACK_PUSH(Stack, Var) Stack = (UINT64)Stack - 8; *(UINT64*)(Stack) = (UINT64)Var
#define KERNELRUNTIMEDLLW L"kernelruntime.dll"
#define KERNELRUNTIMEDLL "kernelruntime.dll"


RFPROCESS KERNELAPI CreateProcess(RFPROCESS ParentProcess, LPWSTR ProcessName, UINT64 Subsystem, UINT16 OperatingMode);
RFPROCESS KERNELAPI GetProcessById(UINT64 ProcessId);

BOOL KERNELAPI SetProcessName(RFPROCESS Process, LPWSTR ProcessName);


RFTHREAD KERNELAPI CreateThread(RFPROCESS Process, UINT64 StackSize, THREAD_START_ROUTINE StartAddress, UINT64 Flags, UINT64* ThreadId); // parameters
KERNELSTATUS KERNELAPI SetThreadPriority(RFTHREAD Thread, int Priority);
KERNELSTATUS KERNELAPI SetPriorityClass(RFPROCESS Process, int PriorityClass);


int KERNELAPI TerminateCurrentProcess(int ExitCode);
int KERNELAPI TerminateCurrentThread(int ExitCode);
int KERNELAPI TerminateProcess(RFPROCESS process, int ExitCode);
int KERNELAPI TerminateThread(RFTHREAD thread, int ExitCode);

RFPROCESS KERNELAPI GetCurrentProcess();
RFTHREAD KERNELAPI GetCurrentThread();
RFTHREAD KERNELAPI GetProcessorIdleThread(UINT64 ProcessorId);

UINT64 KERNELAPI GetCurrentProcessId();
UINT64 KERNELAPI GetCurrentThreadId();

BOOL KERNELAPI SuspendThread(RFTHREAD Thread);
BOOL KERNELAPI ResumeThread(RFTHREAD Thread);

extern PROCESSMGRTABLE Pmgrt;
extern void ScheduleTask();
extern void SkipTaskSchedule();


double KERNELAPI GetThreadCpuTime(RFTHREAD Thread);
double KERNELAPI GetIdleCpuTime();
double KERNELAPI GetTotalCpuTime();
double KERNELAPI GetProcessorIdleTime(UINT64 ProcessorId);
double KERNELAPI GetProcessorTotalTime(UINT64 ProcessorId);

BOOL SetThreadProcessor(RFTHREAD Thread, UINT ProcessorId);


void KERNELAPI TaskSchedulerEnable();
void KERNELAPI TaskSchedulerDisable();

BOOL KERNELAPI IoWait();
BOOL KERNELAPI IoFinish(RFTHREAD Thread);

BOOL isValidProcess(RFPROCESS Process);