#pragma once
#include <krnltypes.h>
#include <CPU/paging_defs.h>
#include <MemoryManagement.h>
#include <Management/handle.h>
#include <Management/lock.h>


#define PENTRIES_PER_LIST 100

#define USER_IMAGE_BASE             0x250000
#define USER_MAIN_IMAGE_LIMIT   0x5F00000000
#define USER_IMAGE_DLL_BASE     0x6000000000
#define USER_IMAGE_LIMIT        0x8F00000000
#define USER_STACK_BASE         0xFF00000000
#define USER_STACK_LIMIT        0x9000000000
#define USER_HEAP_BASE          0x10000000000
#define USER_HEAP_LIMIT         0x70000000000
#define USER_SYSTEM_BASE         0x7F000000000
#define VIRTUAL_ADDRESS_LIMIT   0xFFFFFFFFFFF


#define PROCESS_SYS_DATA_BASE 0x100000
#define END_PROCESS_RETURN_CODE 0xFC7F0000
#define PROCESS_CREATION_FAILED_EXIT_CODE 0xFC7F0000



#define CS_KERNEL 0x08
#define DS_KERNEL 0x10

#define _OFFSET_CS_USER 0x28
#define _OFFSET_DS_USER 0x30

#define KERNELMODE_PROCESS 0x10
#define USERMODE_PROCESS 0x20


#define CS_USER _OFFSET_CS_USER | 3 // segment selector
#define DS_USER _OFFSET_DS_USER | 3 // segment selector

#define THREAD_DEFAULT_STACK_SIZE 0x10000
#define PROCESS_DEFAULT_HEAP_SIZE 0x10000

#define PNAME_CHARSZ_MAX 50

#define PDATA_LIST_SIZE 25

#include <CPU/regs.h>

#pragma pack(push, 1)

enum SUBSYSTEMS {
    SUBSYSTEM_UNKNOWN = 0,
    SUBSYSTEM_NATIVE = 1,
    SUBSYSTEM_GUI = 2,
    SUBSYSTEM_CONSOLE = 3,
    SUBSYSTEM_MAX = 3
};

enum THREAD_STATE{
    THS_PRESENT = 1,
    THS_RUNNABLE = 2,
    THS_ALIVE = 4,
    THS_IOWAIT = 8,
    THS_DONE = 0x10,
    THS_REGISTRED = 0x20,
    THS_IDLE = 0x40, // Idle Thread
    THS_TERMINATED = 0x80,
    THS_IOPIN = 0x100, // IO is sended to the thread & IOWAIT will be removed,
    THS_SLEEP = 0x200, // Thread is in sleeping state
    THS_MANUAL = 0x400
};
enum PRIORITY_CLASS{
    PRIORITY_CLASS_MIN = 0x17E,
    PRIORITY_CLASS_REALTIME = 0x17E,
    PRIORITY_CLASS_HIGH = 0x17F,
    PRIORITY_CLASS_ABOVE_NORMAL = 0x180,
    PRIORITY_CLASS_NORMAL = 0x181,
    PRIORITY_CLASS_BELOW_NORMAL = 0x182,
    PRIORITY_CLASS_LOW = 0x183,
    PRIORITY_CLASS_IDLE = 0x184,
    PRIORITY_CLASS_MAX = 0X184
};
enum THREAD_PRIORITY{
    THREAD_PRIORITY_MIN = 0x25F,
    THREAD_PRIORITY_TIME_CRITICAL = 0x25F,
    THREAD_PRIORITY_HIGH = 0x260,
    THREAD_PRIORITY_ABOVE_NORMAL = 0x261,
    THREAD_PRIORITY_NORMAL = 0x262,
    THREAD_PRIORITY_BELOW_NORMAL = 0x263,
    THREAD_PRIORITY_LOW = 0x264,
    THREAD_PRIORITY_IDLE = 0x265,
    THREAD_PRIORITY_MAX = 0x265
};



typedef struct _THREAD_PRIORITY_PTR_LIST {
    UINT32 Index;
    UINT32 IndexMin;
    UINT32 IndexMax;
    UINT32 Count;
    UINT32 PrioritySpecifier;
    LPVOID threads[PENTRIES_PER_LIST];
    struct _THREAD_PRIORITY_PTR_LIST* Next;
} THPTRLIST, *HTPTRLIST;

typedef struct __TASK_SCHEDULER_DATA_TABLE{
    UINT64 ProcessingRegister0;
    UINT64 ProcessingRegister1;
    UINT64 rip;
    UINT64 cs;
    UINT64 ss;
    UINT64 rflags;
    UINT64 rsp;
    UINT64 cr3;
    UINT64 reserved;
} _TSDT;


enum THREAD_CREATION_FLAGS {
    THREAD_CREATE_SUSPEND = 1,
    THREAD_CREATE_IMAGE_STACK_SIZE = 2,
    THREAD_CREATE_SYSTEM_IDLE = 4,
    THREAD_CREATE_MANUAL = 8
};



enum PROCESS_TOKEN_PRIVILEGES{
    TPPR_ALL = 0xFFFFFFFFFFFFFFFF,
    TPPR_SYSTEM_FILES = 1,
    TPPR_READ_PROCESS_MEMORY = 2,
    TPPR_WRITE_PROCESS_MEMORY = 4,
    TPPR_RAW_DISK_READ = 8,
    TPPR_RAW_DISK_WRITE = 0x10,
    TPPR_PROCESS_CREATE = 0x20,
    TPPR_THREAD_CREATE = 0x40,
    TPPR_PROCESS_TERMINATE = 0x80,
    TPPR_DUPLICATE_HANDLE = 0x100,
    TPPR_QUERY_INFORMATION = 0x200,
    TPPR_QUERY_LIMITED_INFORMATION = 0x400,
    TPPR_SUSPEND_RESUME = 0x800,
    TPPR_SET_INFORMATION = 0x1000,
    TPPR_SYNCHRONIZE = 0x2000,
    TPPR_VM_OPERATION = 0x4000,
    TPPR_VM_READ = 0x8000,
    TPPR_VM_WRITE = 0x10000,
    TPPR_REGISTRY = 0x20000,
    TPPR_REGISTER_DRIVER = 0x40000
};

typedef struct _THREAD_CONTROL_BLOCK THREAD, * RFTHREAD;



typedef struct _THREAD_WAITING_QUEUE THREAD_WAITING_QUEUE, *RFTHREAD_WAITING_QUEUE;

#define THREADS_PER_PROCESS_THREAD_LIST 20

typedef struct _PROCESS_THREAD_LIST PROCESS_THREAD_LIST;

typedef struct _PROCESS_THREAD_LIST {
    RFTHREAD Threads[THREADS_PER_PROCESS_THREAD_LIST];
    PROCESS_THREAD_LIST* Next;
} PROCESS_THREAD_LIST;

typedef enum _PROCESS_CONTROL_MUTEX0_BITS {
    // This also groups CHANGE_PRIORITY class
    // because in order to change priority
    // kernel must reset thread priority for all threads
    // and creating new threads may miss that
    PROCESS_MUTEX0_CREATE_THREAD = 0,
    PROCESS_MUTEX0_ALLOCATE_FREE_HEAP_DESCRIPTOR = 1

} PROCESS_CONTROL_MUTEX0_BITS;

typedef struct _PROCESS_CONTROL_BLOCK{
    BOOL Set;
    UINT64 ProcessId;
    LPWSTR ProcessName;
    RFPAGEMAP PageMap;
    struct _PROCESS_CONTROL_BLOCK* ParentProcess;
    UINT16 OperatingMode;
    UINT32 PriorityClass;
    UINT8 Subsystem;
    UINT64 TokenPrivileges;
    MEMORY_MANAGEMENT_TABLE MemoryManagementTable;
    UINT64 StackSize;
    UINT64 NumThreads;
    UINT64 CpuTime;
    UINT64 SchedulerCpuTime; // The cpu time that the task scheduler incerements
    LPVOID ImageHandle;
    LPVOID DllBase;
    RFTHREAD StartupThread;
    HANDLE_TABLE* Handles;
    HANDLE_TABLE* FileHandles;
    PROCESS_THREAD_LIST Threads;
    UINT64 ControlMutex0;
} PROCESS, *RFPROCESS;

// Bit offsets of the mutexes
typedef enum _THREAD_CONTROL_MUTEX_BITS {
    THREAD_MUTEX_CHANGE_PROCESSOR = 0,
    THREAD_MUTEX_CHANGE_PRIORITY = 1
} THREAD_CONTROL_MUTEX_BITS;

typedef struct _THREAD_CONTROL_BLOCK{
    UINT64       State;
    UINT64      ThreadId;
    RFPROCESS    Process;
    UINT32      ProcessorId; // the cpu that taked control of the thread
    UINT32      ThreadPriority; // Thread Priority Index, Thread Priority Set = Thread Priority - THREAD_PRIORITY_MIN
    UINT32      TimeBurst; // Clock Base Count (Set and decremented in RemainingClocks)
    UINT32      ThreadWaitingQueueIndex;
    RFTHREAD_WAITING_QUEUE WaitingQueue;
    UINT32      SchedulingQuantum; // Added to current num clocks on each preemption
    UINT64      ReadyAt[2]; // The clock number on which the thread will be ready to run
    UINT64      ThreadControlMutex;
    INT64       ExitCode;
    struct CPU_REGISTERS_X86_64 Registers;
    LPVOID      Stack;
    UINT64      CpuTime;
    LPVOID      VirtualStackPtr;
    LPVOID      Client;
    UINT        RemoveIoWaitAfter;
    UINT        AttemptRemoveIoWait; // increments until = RemoveIoWaitAfter (IOWAIT) Flag gets removed
    UINT        RemainingClocks; // Remaining Cpu Time Clocks
    UINT64      SleepUntil[2];
    UINT64      LastCalculationTime[2]; // Second on which the scheduler have set cpu time
} THREAD, *RFTHREAD;
typedef struct _THREAD_LIST{
    THREAD threads[PENTRIES_PER_LIST];
    struct _THREAD_LIST* Next;
} THREADLIST, *RFTHREADLIST;

typedef struct _PROCESS_LIST{
    INT16 LastUnset;
    PROCESS processes[PENTRIES_PER_LIST];
    struct _PROCESS_LIST* Next;
} PLIST, *HPLIST;
enum PRIORITY_CLASS_LIST_INDEX {
    PRIORITY_CLASS_INDEX_IDLE = 6,
    PRIORITY_CLASS_INDEX_LOW = 5,
    PRIORITY_CLASS_INDEX_BELOW_NORMAL = 4,
    PRIORITY_CLASS_INDEX_NORMAL = 3,
    PRIORITY_CLASS_INDEX_ABOVE_NORMAL = 2,
    PRIORITY_CLASS_INDEX_HIGH = 1,
    PRIORITY_CLASS_INDEX_REALTIME = 0,
    PRIORITY_CLASS_INDEX_COUNT = 7
};



typedef struct __PROCESS_MANAGER_TABLE {
    BOOL SchedulerEnable;
    BOOL CpuTimeCalculation; // Set when calculating cpu time <illegal for scheduller to increment cpu time>
    UINT64 NumProcessors;
    THREADLIST ThreadList;
    PLIST ProcessList;
    BOOL SystemInitialized;
    UINT64 NextThreadProcessor; // Used by create thread to chose the threads start processor
    BOOL BroadCastIpi;
    UINT BroadCastIpiCommand;
    UINT64 BroadCastNumProcessors; // Number of processors received the broadcast
    RFTHREAD TargetSuspensionThread; // Used by thread suspend IPI
} PROCESSMGRTABLE, *HPMGRT;


typedef struct _CPU_PERFORMANCE_DESCRIPTOR{
    UINT ThreadTimeCounter[PRIORITY_CLASS_INDEX_COUNT];
    UINT TimerLatencyMultiplier;
    UINT64 MinClock;
    UINT MinTimerClock;
} CPU_PERFORMANCE_DESCRIPTOR;

enum CPU_PERFORMANCE {
    CPU_PERFORMANCE_ECONOMY = 0,
    CPU_PERFORMANCE_VERY_LOW = 1,
    CPU_PERFORMANCE_LOW = 2,
    CPU_PERFORMANCE_BELOW_NORMAL = 3,
    CPU_PERFORMANCE_NORMAL = 4,
    CPU_PERFORMANCE_ABOVE_NORMAL = 5,
    CPU_PERFORMANCE_HIGH = 6,
    CPU_PERFORMANCE_VERY_HIGH = 7,
    CPU_PERFORMANCE_REALTIME = 8,
    CPU_PERFORMANCE_EXTREME = 9,
};


static CPU_PERFORMANCE_DESCRIPTOR CpuPerformanceDescriptors[10] = {
    {
        {50, 45, 40, 30, 20, 15, 10},
        3,
        0,
        0
    },
    {
        {50, 45, 40, 30, 20, 15, 10},
        3,
        0,
        0
    }
    ,
    {
        {50, 45, 40, 30, 20, 15, 10},
        2,
        0,
        0
    }
    ,
    {
        {50, 45, 40, 30, 20, 15, 10},
        2,
        0,
        0
    }
    ,{0},{0},{0},{0},{0},{0}
};

#pragma pack(pop)