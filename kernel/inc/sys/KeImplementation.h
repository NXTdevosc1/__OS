#pragma once
#include <krnltypes.h>
#include <CPU/process.h>
#include <kernel.h>
#include <ipc/ipc.h>

typedef UINT64 LPARAM, HPARAM;
typedef void* DPARAM;
typedef PROCESS* RFPROCESS;
typedef THREAD* RFTHREAD;

enum KERNELEXPORT_IPCIMPLEMENTATION_ID {
	KEX_CREATE_PROCESS = 0x80000,
};
// DParam Create Process
typedef struct _CREATE_PROCESS_STRUCTURE {
	RFPROCESS Process;
	RFPROCESS ParentProcess;
	LPWSTR	  DisplayName;
	LPWSTR    DisplayDescription;
	UINT      SubSystem;
	UINT64    Privileges;
	RFPROCESS* ReturnedProcess;
	UINT64* ProcessId;
} CREATE_PROCESS_STRUCTURE;

#define KEXAPI KERNELAPI // Kernel Export Api

RFPROCESS KEXAPI KeCreateProcess(RFPROCESS ParentProcess, LPWSTR DisplayName, LPWSTR DisplayDescription, UINT SubSystem, UINT64 Privileges, UINT64* ProcessId);