#pragma once
#include <stdint.h>

typedef struct _CPU_MANAGEMENT_TABLE CPU_MANAGEMENT_TABLE;

void _disable(void);
void _enable(void);

#pragma intrinsic(_disable)
#pragma intrinsic(_enable)

#define __cli() _disable()
#define __sti() _enable()

#include <CPU/paging.h>
#include <CPU/descriptor_tables.h>
#include <interrupt_manager/idt.h>
#include <CPU/cpuid.h>
#include <sys/sys.h>
#include <kernel.h>

// Intrinsic used in MSVC CL (directly inserted in code without function call)

#define SMP_BOOT_ADDR (void*)0x8000
#define LAPIC_PAGE_COUNT 120
#define LAPIC_ADDRESS ((UINT64)SystemSpaceBase + SYSTEM_SPACE48_LAPIC)
#define LAPIC_TSC_DEADLINE_MODE_SUPPORT (1<<24)
#define LAPIC_TIMER_ONESHOT_MODE (0 << 17)
#define LAPIC_TIMER_PERIODIC_MODE (1<<17)
#define LAPIC_TIMER_TSC_DEADLINE_MODE (1 << 18)

#define LAPIC_TIMER_SYSTEM_INITIAL_COUNT 0x500
#define LAPIC_TIMER_SYSTEM_DIVISOR 0b1001 // divide bus clock by 64


#define IPI_INTVEC 0x9E // Ipi interrupt vector
#define IPI_SHUTDOWN 0
#define IPI_THREAD_SUSPEND 1
#define INT_SCHEDULE 0x9F // User accessible interrupt to switch to another thread (for e.g. in case of IO)
#define INT_APIC_SPURIOUS 0xFF

#define ALIGN_VALUE(Val, Align) ((Val & (Align - 1)) ? (((Val + Align) & ~(Align - 1))) : (Val))

#define IA32_TSC_DEADLINE_MSR 0x6E0
#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_ENABLE 0x800

// APIC Interrupts
#define INT_APIC_TIMER 0x40
#define INT_TSR 0x41
#define INT_PMCR 0x42
#define INT_LINT0 0x43
#define INT_LINT1 0x44
#define INT_LVT_ERROR 0x45
#define INT_CMCI 0x46

#define CPU_MGMT_BITSHIFT 15
#define CPU_MGMT_NUM_PAGES ((1 << CPU_MGMT_BITSHIFT) >> 12)
void ApicThermalSensorInt(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf);
void ApicPerformanceMonitorCountersInt(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf);
void ApicLint0Int(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf);
void ApicLint1Int(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf);
void ApicLvtErrorInt(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf);
void ApicCmciInt(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf);
void ApicSpuriousInt(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf);

UINT64 ApicTimerBaseQuantum; // Quantum for each thread base time slice
UINT64 ApicTimerClockCounter;
UINT64 TimerClocksPerSecond;
UINT32 TimerIncrementerCpuId;
UINT64 ApicTimerClockQuantum;
enum LAPIC_TIMER_CONFIG {
    LAPIC_TIMER_DIVISOR = 0x3E0,
    LAPIC_TIMER_INITIAL_COUNT = 0x380,
    LAPIC_TIMER_CURRENT_COUNT = 0x390,
    LAPIC_TIMER_LVT = 0x320
};
enum CPU_LOCAL_APIC_REGISTERS{
    CPU_LAPIC_ID = 0x20,
    CPU_LAPIC_VERSION = 0x30,
    CPU_LAPIC_TASK_PRIORITY = 0x80,
    CPU_LAPIC_ARBITRATION_PRIORITY = 0x90,
    CPU_LAPIC_PROCESSOR_PRIORITY = 0xA0,
    CPU_LAPIC_END_OF_INTERRUPT = 0x0B0, // EOI
    CPU_LAPIC_REMOTE_READ = 0xC0,
    CPU_LAPIC_LOGICAL_DESTINATION_REGISTER = 0xD0,
    CPU_LAPIC_DESTINATION_FORMAT_REGISTER = 0x0E0,
    CPU_LAPIC_SPURIOUS_INTERRUPT_VECTOR = 0x0F0,
    CPU_LAPIC_IN_SERVICE_REGISTER = 0x100,
    CPU_LAPIC_IN_TRIGGER_MODE_REGISTER = 0x180,
    CPU_LAPIC_INTERRUPT_REQUEST_REGISTER = 0x200,
    CPU_LAPIC_ERROR_STATUS = 0x280,
    CPU_LAPIC_LVT_CMCI = 0x2F0,
    CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW = 0x300,
    CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_HIGH = 0x310,
    CPU_LAPIC_LVT_TIMER = 0x320,
    CPU_LAPIC_LVT_THERMAL_SENSOR = 0x330,
    CPU_LAPIC_PERFORMANCE_MONITORING_COUNTERS = 0x340,
    CPU_LAPIC_LVT_LINT0 = 0x350,
    CPU_LAPIC_LVT_LINT1 = 0x360,
    CPU_LAPIC_LVT_ERR = 0x370,
    CPU_LAPIC_TIMER_INIT_CNT = 0x380,
    CPU_LAPIC_TIMER_CURRENT_CNT = 0x390,
    CPU_LAPIC_TIMER_DIV = 0x3E0,
    CPU_LAPIC_LAST = 0x38F,
    CPU_LAPIC_DISABLE = 0x10000,
    CPU_LAPIC_SW_ENABLE = 0x100,
    CPU_LAPIC_CPU_FOCUS = 0x200,
    CPU_LAPIC_NMI = 0x400,
    CPU_LAPIC_TIMER_PERIODIC = 0x20000,
    CPU_LAPIC_TIMER_BASE_DIVISOR = 0x100000
};

#pragma pack(push, 1)
struct CPU_IDENTIFICATION_TABLE{
    uint8_t sse3_support : 1;
    uint8_t vmx_support : 1;
    uint8_t enhanced_intel_speed_step : 1;
    uint8_t sse4_1 : 1;
    uint8_t sse4_2 : 1;
    uint8_t tsc_deadline_support : 1;
    uint8_t aes_support : 1;
    uint8_t xsave_xstor : 1;
    uint8_t osxsave : 1; // extended states
    uint8_t avx_support;
    uint8_t rdrand_support : 1;
};

struct CPU_CORE_INFO{
	void* lapic;
	uint8_t active;
	struct CPU_IDENTIFICATION_TABLE identification_table;
};

#pragma pack(pop)

#define _USER_SYSTEM_CONFIG_ADDRESS 0x100000

typedef struct _USER_SYSTEM_CONFIG {
    UINT64 PreferredSysenterMethod; // An index in the EnterSystem Wrapper to the sysenter/syscall function

} USER_SYSTEM_CONFIG;

void InitProcessorDescriptors(void** CpuBuffer, UINT64* CpuBufferSize);

void KEXPORT KERNELAPI Sleep(UINT64 Milliseconds);
void KEXPORT KERNELAPI MicroSleep(UINT64 Microseconds);

KERNELSTATUS InitializeApicCpu(UINT64 ApicId);
void SetupLocalApicTimer();

void DeclareCpuHalt();

void CpuSetupManagementTable(UINT64 CpuCount);

extern void SMP_TRAMPOLINE();
extern uint64_t SMP_TRAMPOLINE_END;

extern void EnableExtendedStates();

// Setup booted SMP Cpu's

extern void __Schedule(); // throw an INT_SCHEDULE

extern void __ldmxcsr(unsigned int mxcsr);
extern unsigned int __stmxcsr();

extern void SetupCPU();

extern void __InvalidatePage(void* Address);

extern unsigned long long __rdtsc();

extern void __wbinvd(); // write back invalidate cache

extern void __repstos(void* Address, unsigned char Value, unsigned long long Count);
extern void __repstos16(void* Address, unsigned short Value, unsigned long long Count);
extern void __repstos32(void* Address, unsigned int Value, unsigned long long Count);
extern void __repstos64(void* Address, unsigned long long Value, unsigned long long Count);

void BroadcastInterrupt(char InterruptNumber, BOOL SelfSend);
void SendProcessorInterrupt(UINT ApicId, char InterruptNumber);

void IpiSend(UINT ApicId, UINT Command);
void IpiBroadcast(UINT Command, BOOL SelfSend);

void EnableApic();
void SetLocalApicBase(void* _Lapic);

extern struct GDT gdt;
extern struct GDT_DESCRIPTOR gdtr;

extern USER_SYSTEM_CONFIG GlobalUserSystemConfig;
#pragma pack(push, 1)

BOOL __CPU_MGMT_TBL_SETUP__;

#define NUM_IRQS_PER_PROCESSOR 0xE0 // IRQs 0-24 (IOAPIC), 24-0xE0 MSI/MSI-X/System Interrupts/User mode interrupts

typedef struct _THREAD_CONTROL_BLOCK THREAD, *RFTHREAD;

#define NUM_PRIORITY_CLASSES 7

// Num threads must be 64 byte aligned
// for SSE/AVX/AVX512 Optimizations
#define NUM_THREADS_PER_WAITING_QUEUE 0x200


typedef struct _THREAD_WAITING_QUEUE THREAD_WAITING_QUEUE, *RFTHREAD_WAITING_QUEUE;

typedef struct _THREAD_WAITING_QUEUE {
    RFTHREAD Threads[NUM_THREADS_PER_WAITING_QUEUE];
    RFTHREAD_WAITING_QUEUE NextQueue;
    UINT NumThreads;
    BOOL Mutex;
} THREAD_WAITING_QUEUE, *RFTHREAD_WAITING_QUEUE;

typedef struct _CPU_MANAGEMENT_TABLE {
    // Task Scheduler Specific
    BOOL Initialized;
    UINT32 ProcessorId;
    UINT64 Reserved5; // For alignment
    UINT64 TotalThreads[NUM_PRIORITY_CLASSES];
    UINT64 Reserved0;
    UINT64 NumReadyThreads[NUM_PRIORITY_CLASSES];
    UINT64 Reserved1;
    RFTHREAD_WAITING_QUEUE ThreadQueues[NUM_PRIORITY_CLASSES];
    UINT64 Reserved7;
    RFTHREAD CurrentThread;
    RFTHREAD SelectedThread;
    UINT64 TotalClocks[2]; // 128-bit value
    UINT64 ReadyOnClock[NUM_PRIORITY_CLASSES * 2]; // 128 Bit
    UINT64 Reserved3[2];
    UINT64 HighestPriorityThread[NUM_PRIORITY_CLASSES];
    UINT64 Reserved4;
    RFTHREAD SystemIdleThread;
    RFTHREAD SystemInterruptsThread;
    RFTHREAD ForceNextThread;
    UINT64 CurrentCpuTime;
    UINT64 EstimatedCpuTime; // Set to estimated cpu time on each second by scheduler
    // System (Kernel) Specific
    UINT64 Reserved6;
    UINT64 HighPrecisionTime[2];
    UINT64 LastThreadSwitchLatency[2];
    IRQ_CONTROL_DESCRIPTOR IrqControlTable[NUM_IRQS_PER_PROCESSOR];
    UINT IpiCommand;
    BOOL IpiCommandControl;
    void* CpuBuffer;
    UINT64 CpuBufferSize;
    
} CPU_MANAGEMENT_TABLE;

CPU_MANAGEMENT_TABLE** CpuManagementTable;

#pragma pack(pop)
extern KERNELSTATUS CpuBootStatus;
extern void _Xmemset128(void* ptr, unsigned long long val, size_t size);
extern void _Xmemset256(void* ptr, unsigned long long val, size_t size);
extern void _Xmemset512(void* ptr, unsigned long long val, size_t size);

// extern void __cli();
// extern void __sti();
extern void __hlt();
extern void __pause();

extern void __setCR3(unsigned long long CR3);
extern unsigned long long __getCR3();

extern void __lidt(void* idtr);
extern void __lgdt(void* gdtr);
void __cpuidex(CPUID_INFO* CpuInfo, int FunctionId, int SubfunctionId);

extern void __ReadMsr(unsigned int msr, unsigned int* eax, unsigned int* edx);
extern void __WriteMsr(unsigned int msr, unsigned int eax, unsigned int edx);

extern unsigned int GetCurrentProcessorId();
extern unsigned long long __getCR2();
extern void __setRFLAGS(unsigned long long RFLAGS);
extern unsigned long long __getRFLAGS();


extern void __SpinLockSyncBitTestAndSet(void* Address, UINT16 BitOffset);
extern BOOL __SyncBitTestAndSet(void* Address, UINT16 BitOffset); // return 1 if success, 0 if fail
// Previous bit content is storred in carry flag (__getRFLAGS()) to test content of CF
extern void __BitRelease(void* Address, UINT16 BitOffset); // Does not need synchronization

extern void __SyncIncrement64(UINT64* Address);
extern void __SyncIncrement32(UINT32* Address);
extern void __SyncIncrement16(UINT16* Address);
extern void __SyncIncrement8(UINT8* Address);

extern void __SyncDecrement64(UINT64* Address);
extern void __SyncDecrement32(UINT32* Address);
extern void __SyncDecrement16(UINT16* Address);
extern void __SyncDecrement8(UINT8* Address);