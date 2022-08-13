/*
COPYRIGHT NOTICE
...
*/

/* This is the header for drivers compiled with the DLL : kernelruntime.dll
 * kernelruntime.dll is a virtual DLL to link driver with kernelruntime symbols
 * when the operating system detects kernelruntime.dll, it does not load the DLL But links the driver with kernel symbols
 */
#pragma once
#include <krnlapi.h>
#ifndef __KERNEL
#include <kerneltypes.h>


typedef union _INTERRUPT_STACK_FRAME INTERRUPT_STACK_FRAME, *RFINTERRUPT_STACK_FRAME;

#pragma pack(push, 1)

typedef union _INTERRUPT_STACK_FRAME {
    struct {
        UINT64	InstructionPointer;
        UINT64	CodeSegment;
        UINT64	CpuRflags;
        UINT64	StackPointer;
        UINT64	StackSegment;
    } IntStack;
    struct {
        UINT64 InterruptCode;
        UINT64	InstructionPointer;
        UINT64	CodeSegment;
        UINT64	CpuRflags;
        UINT64	StackPointer;
        UINT64	StackSegment;
    } CodeIntStack; // Interrupt stack passing code
} INTERRUPT_STACK_FRAME, *RFINTERRUPT_STACK_FRAME;

typedef enum _IRQ_DELIVERY_MODE{
    IRQ_DELIVERY_NORMAL = 0,
    IRQ_DELIVERY_SMI,
    IRQ_DELIVERY_NMI,
    IRQ_DELIVERY_EXTINT
} IRQ_DELIVERY_MODE;

#define INTERRUPT_SOURCE_USER 0 // User accessible interrupt
#define INTERRUPT_SOURCE_IRQ 1
#define INTERRUPT_SOURCE_IRQMSI 2
#define INTERRUPT_SOURCE_SYSTEM 3 // Like multiprocessor interrupts

typedef void* RFDEVICE_OBJECT;

typedef struct _INTERRUPT_INFORMATION {
    RFDEVICE_OBJECT Device;
    UINT32 Source;
    RFTHREAD PreviousThread; // Mainly used in interrupt_source_user
    RFINTERRUPT_STACK_FRAME InterruptStack;
} INTERRUPT_INFORMATION, *RFINTERRUPT_INFORMATION;

#pragma pack(pop)

typedef struct _DRIVER_OBJECT DRIVER_OBJECT, *RFDRIVER_OBJECT;

typedef void (__cdecl* INTERRUPT_SERVICE_ROUTINE)(_IN void* Driver, _IN void* InterruptInformation);

#endif

RFPROCESS KERNELAPI KeCreateProcess(RFPROCESS ParentProcess, LPWSTR ProcessName, UINT64 Subsystem, UINT16 SegmentBase);
RFTHREAD KERNELAPI KeCreateThread(RFPROCESS Process, UINT64 StackSize, THREAD_START_ROUTINE StartAddress, UINT64 Flags, UINT64* LpThreadId, UINT8 NumParameters, ...); // parameters


RFPROCESS KERNELAPI KeGetProcessById(UINT64 ProcessId);

BOOL KERNELAPI KeSetProcessName(RFPROCESS Process, LPWSTR ProcessName);



HRESULT KERNELAPI KeSetThreadPriority(RFTHREAD Thread, int Priority);
HRESULT KERNELAPI KeSetPriorityClass(RFPROCESS Process, int PriorityClass);


void KERNELAPI KeTerminateCurrentProcess(int ExitCode);
void KERNELAPI KeTerminateCurrentThread(int ExitCode);
int KERNELAPI KeTerminateProcess(RFPROCESS process, int ExitCode);
int KERNELAPI KeTerminateThread(RFTHREAD thread, int ExitCode);

RFPROCESS KERNELAPI KeGetCurrentProcess();
RFTHREAD KERNELAPI KeGetCurrentThread();

UINT64 KERNELAPI KeGetCurrentProcessId();
UINT64 KERNELAPI KeGetCurrentThreadId();

BOOL KERNELAPI KeSuspendThread(RFTHREAD Thread);
BOOL KERNELAPI KeResumeThread(RFTHREAD Thread);
void KERNELAPI KeSetDeathScreen(UINT64 DeathCode, UINT16* Msg, UINT16* Description, UINT16* SupportLink);

PVOID KERNELAPI KeGetRuntimeSymbol(char* SymbolName);


void KERNELAPI KeTaskSchedulerEnable();
void KERNELAPI KeTaskSchedulerDisable();

PVOID KERNELAPI malloc(unsigned long long Size);
LPVOID KERNELAPI AllocatePoolEx(HTHREAD Thread, UINT64 NumBytes, UINT32 Align, LPVOID* AllocationSegment, UINT64 MaxAddress);
PVOID KERNELAPI free(const void* Heap);

// IO_MEMORY Automatically mapped with NX(No-Execute) 
LPVOID KERNELAPI AllocateIoMemory(LPVOID PhysicalAddress, UINT64 NumPages, UINT Flags);
BOOL KERNELAPI FreeIoMemory(LPVOID IoMem);

#define OS_SUPPORT_LNKW L"os_support.com"
#define OS_SUPPORT_LNK "os_support.com"

enum PAGEMAP_FLAGS{
    PM_PRESENT = 1,
    PM_USER = 2,
    PM_READWRITE = 4,
    PM_MAP = PM_PRESENT | PM_READWRITE,
    PM_UMAP = PM_MAP | PM_USER,
    PM_UNMAP = 0,
    PM_NX = 8, // No execute Enable
    PM_GLOBAL = 0x10,
    PM_CACHE_DISABLE = 0x20,
    PM_WRITE_THROUGH = 0x40 // Write in the cache and also in memory
};

int KERNELAPI KeMapProcessMemory(RFPROCESS Process, void* PhysicalAddress, void* VirtualAddress, UINT64 NumPages, UINT64 Flags);
int KERNELAPI KeMapMemory(void* PhysicalAddress, void* VirtualAddress, UINT64 NumPages, UINT64 Flags);

KERNELSTATUS KERNELAPI SystemDebugPrint(LPCWSTR Format, ...);

/*
- KeRegisterIsr
Return Value:
SUCCESS : KERNEL_SOK
ERROR : KERNEL_SERR_OUT_OF_RANGE (ISR Number exceeds max IRQs supported by the Computer)
ERROR : KERNEL_SERR_IRQ_CONTROLLED_BY_ANOTHER_PROCESS (Irq registred by another process)
WARNING : KERNEL_SWR_IRQ_ALREADY_SET (ISR Already set by current process)
*/


KERNELSTATUS KERNELAPI KeControlIrq(INTERRUPT_SERVICE_ROUTINE InterruptHandler, UINT IrqNumber, UINT DeliveryMode, UINT Flags);
KERNELSTATUS KERNELAPI KeReleaseIrq(UINT IrqNumber);

typedef void* DEVICE_OBJECT, *RFDEVICE_OBJECT;

// The kernel will choose if the device is eligible for interrupts and whether using MSI/MSI-x or Standard Interrupts
KERNELSTATUS KERNELAPI SetInterruptService(RFDEVICE_OBJECT Device, INTERRUPT_SERVICE_ROUTINE InterruptHandler);

void KERNELAPI Sleep(UINT64 Milliseconds);
void KERNELAPI MicroSleep(UINT64 MicroSeconds);

// Puts the thread on the waiting queue until IoFinish is called by another thread
BOOL KERNELAPI IoWait();

// Removes IO_WAIT Status from the target thread if set
BOOL KERNELAPI IoFinish(HTHREAD Thread);