#pragma once
#include <stdint.h>
#include <CPU/process_defs.h>
#include <loader_defs.h>
#include <krnltypes.h>
#include <ipc/ipc.h>
#include <ipc/ipcserver.h>
#define __DECLARE_FLT extern int _fltused = 0

FILE_IMPORT_ENTRY FileImportTable[]; // Contains all default drivers, modules and files to run the Operating System

extern UINT64 KeGlobalCR3;
extern RFPROCESS kproc;
extern RFPROCESS IdleProcess;
extern RFPROCESS SystemInterruptsProcess;
extern INITDATA InitData;
extern UINT64 _krnlbase, _krnlend;

void KERNELAPI IdleThread();
RFSERVER KernelServer;

enum KeMemMapTypes{
    KeAllocatedMemory = 0x8000
};

#pragma section(_FIMPORT)

#define MAJOR_KERNEL_VERSION 1
#define MINOR_KERNEL_VERSION 0

#define LEN_OS_NAME_MAX 0x60

// FIMPORT_DEFINED_INDEXES
#define FIMPORT_BOOT_CONFIG 0
#define FIMPORT_DRVTBL 1

#define KEHEADER_END L"Ke$ENOFHDR"

#define SYSTEM_CONTROL_PORT_A 0x92
#define SYSTEM_CONTROL_PORT_B 0x61


/*Kernel Space (48-BIT) :
16 TB : APIC & Cpu Management Tables
8 TB : Kernel & Dependencies
8 TB : Processor Tables (GDT, IDT...)
32 TB : I/O Space
64 TB : RAM
*/
// Kernel 48-BIT Address Map (Offset from System Space Base)
#define SYSTEM_SPACE48_LAPIC            0
#define SYSTEM_SPACE48_PMGRT            0x20000
#define SYSTEM_SPACE48_CPU_MGMT         0x200000
#define SYSTEM_SPACE48_KERNEL           0x100000000000
#define SYSTEM_SPACE48_DEPENDENCIES     0x120000000000
#define SYSTEM_SPACE48_INTERRUPT_CODE   0x140000000000
#define SYSTEM_SPACE48_PROCESSOR_TABLES 0x180000000000
#define SYSTEM_SPACE48_IO               0x200000000000
#define SYSTEM_SPACE48_RAM              0x400000000000




PROCESS IoSpaceMemoryProcess; // This is a (non) present process that contains the heap of the IoSpace
THREAD IoSpaceMemoryThread;