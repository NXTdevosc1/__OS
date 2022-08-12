#include <CPU/process.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <kernel.h>
#include <cstr.h>
#include <CPU/cpu.h>
#include <preos_renderer.h>
#include <stddef.h>
#include <mem.h>
#include <stdlib.h>
#include <loaders/subsystem.h>
#include <ipc/ipc.h>
#include <CPU/pcid.h>
#include <Management/debug.h>
__declspec(align(0x1000)) PROCESSMGRTABLE Pmgrt = { 0 };

__declspec(align(0x1000)) UINT64 GlobalThreadPreemptionPriorities[] = {
    // Classified from TIME_CRITICAL to IDLE
    3,
    4,
    5,
    6,
    10,
    15,
    25
};


UINT64 ReducePriorityAfter = 25;
UINT64 ReducePriorityCountDown = 25;


static inline int KERNELAPI SetProcess(
    RFPROCESS Process, UINT64 ProcessId, RFPROCESS ParentProcess,
    UINT16 OperatingMode, LPWSTR ProcessName, UINT16 NameLength, UINT64 Subsystem
){
    Process->Set = 1;
    Process->ParentProcess = ParentProcess;
    Process->OperatingMode = OperatingMode;
    Process->ProcessId = ProcessId;
    Process->Subsystem = Subsystem;
   
    CreateMemoryTable(Process, &Process->MemoryManagementTable);

    if (kproc) {
        LPWSTR NameCpy = kmalloc((NameLength << 1) + 2);
        memcpy16(NameCpy, ProcessName, NameLength);
        NameCpy[NameLength] = 0;
        Process->ProcessName = NameCpy;
        if (Process->OperatingMode != KERNELMODE_PROCESS) {
            Process->PageMap = CreatePageMap();
            if (!Process->PageMap) SET_SOD_PROCESS_MANAGEMENT;
            ProcessContextIdAllocate(Process);
        }
        else {
            Process->PageMap = kproc->PageMap;
        }
        Process->Handles = CreateHandleTable();
        Process->FileHandles = CreateHandleTable();
        switch (Subsystem) {
            case SUBSYSTEM_CONSOLE:
            {
                SetPriorityClass(Process, PRIORITY_CLASS_NORMAL);
                break;
            }
            case SUBSYSTEM_GUI:
            {
                SetPriorityClass(Process, PRIORITY_CLASS_ABOVE_NORMAL);
                break;
            }
            case SUBSYSTEM_NATIVE:
            {
                SetPriorityClass(Process, PRIORITY_CLASS_LOW);
                break;
            }
            default:
            {
                SetPriorityClass(Process, PRIORITY_CLASS_BELOW_NORMAL);
                break;
            }
        }
        
    }
    else {
        Process->ProcessName = ProcessName;
    }
    
    return 0;
}

RFPROCESS KEXPORT KERNELAPI KeCreateProcess(RFPROCESS ParentProcess, LPWSTR ProcessName, UINT64 Subsystem, UINT16 OperatingMode){

    if (!ProcessName || !OperatingMode || Subsystem > SUBSYSTEM_MAX || (OperatingMode != KERNELMODE_PROCESS && OperatingMode != USERMODE_PROCESS)) return NULL;
    UINT16 len = wstrlen(ProcessName);
    if(!len || len > PNAME_CHARSZ_MAX) return NULL;

    HPLIST current = &Pmgrt.ProcessList;
    UINT64 i = 0;

    
    for(;;i++){
        PROCESS* Process = current->processes;
        for(UINT64 c = 0;c<PENTRIES_PER_LIST;c++, Process++){
            if(!Process->Set) {
                
                SetProcess(Process,
                c + i*PENTRIES_PER_LIST, ParentProcess,
                OperatingMode, ProcessName, len, Subsystem
                );
                return Process;
            }
        }
        if(!current->Next) break;
        current = current->Next;
    }
    i++;
    current->Next = kmalloc(sizeof(PLIST));
    if(!current->Next) SET_SOD_PROCESS_MANAGEMENT;
    SZeroMemory(current->Next);
    SetProcess(&current->processes[0], i * PENTRIES_PER_LIST, ParentProcess, OperatingMode, ProcessName, len, Subsystem);
    return &current->processes[0];
}

BOOL KEXPORT KERNELAPI KeSuspendThread(RFTHREAD Thread) {
    if (!Thread) return FALSE;
    Thread->State &= ~(THS_ALIVE);
    //Pmgrt.TargetSuspensionThread = Thread;
    //IpiBroadcast(IPI_THREAD_SUSPEND, TRUE);
    return TRUE;
}
BOOL KEXPORT KERNELAPI KeResumeThread(RFTHREAD Thread) {
    if (!Thread) return FALSE;
    Thread->State |= THS_ALIVE;
    return TRUE;
}

BOOL SetThreadProcessor(RFTHREAD Thread, UINT ProcessorId) {
    if (!Thread || ProcessorId >= Pmgrt.NumProcessors || !CpuManagementTable[ProcessorId]->Initialized)
        return FALSE;

    //SuspendThread(Thread);
    Thread->ProcessorId = ProcessorId;
    //ResumeThread(Thread);
    return TRUE;
}

RFPROCESS KERNELAPI GetProcessById(UINT64 ProcessId){
    HPLIST current = &Pmgrt.ProcessList;
    uint64_t npid = ProcessId;
    for(;;){
        if(npid >= PENTRIES_PER_LIST) {
            npid-=PENTRIES_PER_LIST;
            if(!current->Next) return NULL;
            current = current->Next;
            continue;
        }
        if(current->processes[npid].Set) return &current->processes[npid];
        return NULL;
    }
    return NULL;
}

BOOL KERNELAPI SetProcessName(RFPROCESS Process, LPWSTR ProcessName) {
    if (!Process || !ProcessName) return FALSE;
    UINT16 len = wstrlen(ProcessName);
    if (!len || len > PNAME_CHARSZ_MAX) return FALSE;
    LPWSTR copy = kmalloc(len + 2);
    if (!copy) SET_SOD_MEMORY_MANAGEMENT;
    memcpy16(copy, ProcessName, len);
    copy[len] = 0;

    free(Process->ProcessName, kproc);
    Process->ProcessName = copy;
    return TRUE;
}


KERNELSTATUS KERNELAPI SetThreadPriority(RFTHREAD Thread, int Priority){
#ifdef ___KERNEL_DEBUG___
DebugWrite("SetThreadPriority()");
DebugWrite(to_hstring64((UINT64)Thread));
#endif


    if(!Thread || Priority < THREAD_PRIORITY_MIN || Priority > THREAD_PRIORITY_MAX)
        return KERNEL_SERR_INVALID_PARAMETER;

    Priority -= THREAD_PRIORITY_MIN;
    __SpinLockSyncBitTestAndSet(&Thread->ThreadControlMutex, THREAD_MUTEX_CHANGE_PRIORITY);
    RFTHREAD_WAITING_QUEUE PreviousWaitingQueue = Thread->WaitingQueue;
    UINT PreviousWaitingQueueIndex = Thread->ThreadWaitingQueueIndex;
    CPU_MANAGEMENT_TABLE* CpuMgmt = CpuManagementTable[Thread->ProcessorId];
    if(PreviousWaitingQueue) {
        CpuMgmt->TotalThreads[Thread->Process->PriorityClass]-= 1;
        CpuMgmt->NumReadyThreads[Thread->Process->PriorityClass]--;
    }

    RFTHREAD_WAITING_QUEUE WaitingQueue = CpuMgmt->ThreadQueues[Thread->Process->PriorityClass];
    Thread->ThreadPriority = Priority;
    Thread->SchedulingQuantum = GlobalThreadPreemptionPriorities[Priority];

// Allocate thread in the waiting queue
    for(;;) {
        if(WaitingQueue->NumThreads < NUM_THREADS_PER_WAITING_QUEUE) {
            __SpinLockSyncBitTestAndSet(&WaitingQueue->Mutex, 0);
            if(WaitingQueue->NumThreads == NUM_THREADS_PER_WAITING_QUEUE) {
                __BitRelease(&WaitingQueue->Mutex, 0);
            } else {
                RFTHREAD* _th = WaitingQueue->Threads;
                for(UINT i = 0;i<NUM_THREADS_PER_WAITING_QUEUE;i++, _th++) {
                    if(!(*_th)) {
                        *_th = Thread;
                        Thread->ThreadWaitingQueueIndex = i;
                        Thread->WaitingQueue = WaitingQueue;
                        WaitingQueue->NumThreads++;
                        CpuMgmt->TotalThreads[Thread->Process->PriorityClass]+= 1;
                        CpuMgmt->NumReadyThreads[Thread->Process->PriorityClass]++;

                        __BitRelease(&WaitingQueue->Mutex, 0);
                        goto R0;
                    }
                }
            }
            __BitRelease(&WaitingQueue->Mutex, 0);
        }
        if(!WaitingQueue->NextQueue) {
            WaitingQueue->NextQueue = malloc(sizeof(THREAD_WAITING_QUEUE));
            SZeroMemory(WaitingQueue->NextQueue);
        }
        WaitingQueue = WaitingQueue->NextQueue;
    }
R0:
// Remove thread from previous waiting queue
// No spinlock needed
    if(PreviousWaitingQueue) {
        __SyncDecrement32(&PreviousWaitingQueue->NumThreads);
        PreviousWaitingQueue->Threads[PreviousWaitingQueueIndex] = NULL;
    }
    __BitRelease(&Thread->ThreadControlMutex, THREAD_MUTEX_CHANGE_PRIORITY);
    return KERNEL_SOK;
}
KERNELSTATUS KERNELAPI SetPriorityClass(RFPROCESS Process, int PriorityClass) {
    if(PriorityClass < PRIORITY_CLASS_MIN || PriorityClass > PRIORITY_CLASS_MAX) return KERNEL_SERR_INVALID_PARAMETER;
#ifdef ___KERNEL_DEBUG___
    DebugWrite("SetPriorityClass()");
#endif
    PriorityClass -= PRIORITY_CLASS_MIN;
    __SpinLockSyncBitTestAndSet(&Process->ControlMutex0, PROCESS_MUTEX0_CREATE_THREAD);
    Process->PriorityClass = PriorityClass;
    UINT RemainingThreads = Process->NumThreads;
    PROCESS_THREAD_LIST* ThreadList = &Process->Threads;
    while(RemainingThreads) {
        RFTHREAD* _th = ThreadList->Threads;
        for(UINT i = 0;i<THREADS_PER_PROCESS_THREAD_LIST;i++, _th++) {
            if(*_th) {
                if(KERNEL_ERROR(SetThreadPriority(*_th, (*_th)->ThreadPriority + THREAD_PRIORITY_MIN))) SET_SOD_PROCESS_MANAGEMENT;
                RemainingThreads--;
            }
        }
        ThreadList = ThreadList->Next;
    }
    __BitRelease(&Process->ControlMutex0, PROCESS_MUTEX0_CREATE_THREAD);

    return KERNEL_SOK;
}

static inline void KERNELAPI SetupThreadRegisters(RFTHREAD thread) {
    RFPROCESS process = thread->Process;
    if (process->OperatingMode == KERNELMODE_PROCESS) {
        thread->Registers.cs = CS_KERNEL;
        thread->Registers.ds = DS_KERNEL;
        thread->Registers.es = DS_KERNEL;
        thread->Registers.gs = DS_KERNEL;
        thread->Registers.fs = DS_KERNEL;
    }
    else if (process->OperatingMode == USERMODE_PROCESS) {
        thread->Registers.cs = CS_USER;
        thread->Registers.ds = DS_USER;
        thread->Registers.es = DS_USER;
        thread->Registers.gs = DS_USER;
        thread->Registers.fs = DS_USER;
    }

    thread->Registers.cr3 = (UINT64)process->PageMap;
    
    thread->Registers.Xsave = kpalloc(1);
    if (!thread->Registers.Xsave) SET_SOD_PROCESS_MANAGEMENT;
    ZeroMemory(thread->Registers.Xsave, 0x1000);

    thread->Client = IpcClientCreate(thread, 0, 0); // Use default queue length

    if (!process->NumThreads) {
        process->StartupThread = thread;
    }
    process->NumThreads++;
    #ifdef  ___KERNEL_DEBUG___
	DebugWrite("Thread Registers Are Setup.");
#endif //  ___KERNEL_DEBUG___
}

static inline RFTHREAD KERNELAPI AllocateThread(RFPROCESS Process, UINT64* LpThreadId) {
    RFTHREADLIST ThreadList = &Pmgrt.ThreadList;
    UINT64 ListIndex = 0;
    RFTHREAD thread = NULL;
    for (;; ListIndex++) {
        for (UINT16 i = 0; i < PENTRIES_PER_LIST; i++) {
            thread = &ThreadList->threads[i];

            if (!thread->State) {
                SZeroMemory(thread);
                thread->State = THS_PRESENT | THS_RUNNABLE; // Without making it alive
                thread->ThreadId = i + ListIndex * PENTRIES_PER_LIST;
                if (LpThreadId) *LpThreadId = thread->ThreadId;
                thread->Process = Process;
                SetupThreadRegisters(thread);
                return thread;
            }
        }
        if (!ThreadList->Next){
            ThreadList->Next = kmalloc(sizeof(*ThreadList));
            if(!ThreadList->Next) SET_SOD_MEMORY_MANAGEMENT;
            SZeroMemory(ThreadList->Next);
        }
        ThreadList = ThreadList->Next;
    }
    return thread;
}



RFTHREAD KEXPORT KERNELAPI KeCreateThread(RFPROCESS Process, UINT64 StackSize, THREAD_START_ROUTINE StartAddress, UINT64 Flags, UINT64* ThreadId){ // parameters
    if(!Process || !Process->Set) return NULL;

    __SpinLockSyncBitTestAndSet(&Process->ControlMutex0, PROCESS_MUTEX0_CREATE_THREAD);

    if (!StackSize) StackSize = THREAD_DEFAULT_STACK_SIZE;

    if (StackSize & 0xFFF) {
        StackSize += 0x1000;
        StackSize &= ~0xFFF;
    }

    RFTHREAD Thread = AllocateThread(Process, ThreadId);
    Thread->Process = Process;
    UINT64 ThreadProcessor = Pmgrt.NextThreadProcessor;

    
    Thread->ProcessorId = ThreadProcessor;
    if (ThreadProcessor + 1 >= Pmgrt.NumProcessors) Pmgrt.NextThreadProcessor = 0;
    else Pmgrt.NextThreadProcessor = ThreadProcessor + 1;


    #ifdef  ___KERNEL_DEBUG___
	DebugWrite("Thread Processor Specified. Setting Up Thread's Stack");
#endif //  ___KERNEL_DEBUG___

    if(!StackSize) StackSize = THREAD_DEFAULT_STACK_SIZE;
    if(StackSize % 0x1000) StackSize += 0x1000 - (StackSize % 0x1000);
    if(Process->OperatingMode != KERNELMODE_PROCESS){
        Thread->Stack = kpalloc(StackSize >> 12);
        if (!Thread->Stack) SET_SOD_MEMORY_MANAGEMENT;
        ZeroMemory(Thread->Stack, StackSize);

        Thread->VirtualStackPtr = (LPVOID)(USER_STACK_BASE - Process->StackSize - StackSize);
        Thread->Registers.rsp = (UINT64)USER_STACK_BASE - Process->StackSize;
        Thread->Registers.rbp = Thread->Registers.rsp;
        MapPhysicalPages(Process->PageMap, (LPVOID)(USER_STACK_BASE - Process->StackSize - StackSize), Thread->Stack, StackSize >> 12, PM_UMAP);
    }
    else {
        // Kernel Processes shares the same page table with the kernel
            Thread->Stack = kpalloc(StackSize >> 12);
            if (!Thread->Stack) SET_SOD_MEMORY_MANAGEMENT;
            ZeroMemory(Thread->Stack, StackSize);
            Thread->Registers.rsp = (UINT64)Thread->Stack + StackSize - 0x400;
            Thread->Registers.rbp = (UINT64)Thread->Registers.rsp;
    }

    Thread->Registers.rflags = 0x200; // Interrupts Enable
    

    Process->StackSize += StackSize;
    ThreadWrapperInit(Thread, StartAddress);
    

    
    // Linking the thread to the process
    PROCESS_THREAD_LIST* ThreadList = &Process->Threads;
    for(;;) {
        RFTHREAD* _th = ThreadList->Threads;
        for(UINT i = 0;i<THREADS_PER_PROCESS_THREAD_LIST;i++, _th++) {
            if(!(*_th)) {
                *_th = Thread;
                goto R0;
            }
        }
        if(!ThreadList->Next) {
            ThreadList->Next = malloc(sizeof(PROCESS_THREAD_LIST));
            SZeroMemory(ThreadList->Next);
        }
        ThreadList = ThreadList->Next;
    }
R0:
    if(!(Flags & THREAD_CREATE_SUSPEND)){
        KeResumeThread(Thread);
    }

    Thread->SchedulingQuantum = 10;
    Thread->TimeBurst = 1;


    __BitRelease(&Process->ControlMutex0, PROCESS_MUTEX0_CREATE_THREAD);


    return Thread;

}


int KERNELAPI TerminateCurrentProcess(int exit_code){
    return TerminateProcess(KeGetCurrentProcess(), exit_code);
}
int KERNELAPI TerminateCurrentThread(int exit_code){
    return TerminateThread(KeGetCurrentThread(), exit_code);
}
int KERNELAPI TerminateProcess(RFPROCESS process, int ExitCode){
    return 0;
}
int KERNELAPI TerminateThread(RFTHREAD thread, int ExitCode){
    if (!thread) return -1;

    return 0;
}
// int TerminateProcess(RFPROCESS process, int exit_code){
//     GP_draw_sf_text("Exit code :",0xffffff,20,20);
//     GP_draw_sf_text(to_stringu64(exit_code),0xffffff,20,40);
//     struct PROCESS_DATA_PTR* dataptr = process->threads;
//     for(;;){
//         if(dataptr->present){
//             struct PROCESS_THREAD_TABLE* thread = dataptr->data;
//             if(thread->flags){
//                 thread->flags |= THF_TERMINATED; 
//                 TerminateThread(thread, exit_code);
//             }
//         }
//         if(!dataptr->next) break;
//         dataptr = dataptr->next;
//     }
//     dataptr = process->child_processes;
//     for(;;){
//         if(dataptr->present){
//             RFPROCESS process = dataptr->data;
//             if(process->set){
//                 TerminateProcess(process, exit_code);
//             }
//         }
//         free(dataptr, kproc);
//         dataptr = dataptr->next;
//         if(!dataptr) break;
        
//     }

//     free(process->page_table, kproc);
//     free(process.)
//     while(1);
// }


RFPROCESS KERNELAPI KeGetCurrentProcess(){
    if (!Pmgrt.SystemInitialized) return kproc;
    return KeGetCurrentThread()->Process;
}


RFTHREAD KERNELAPI KeGetCurrentThread(){
    if (!Pmgrt.SystemInitialized) return kproc->StartupThread;
    RFTHREAD Thread = CpuManagementTable[*(UINT32*)((char*)SystemSpaceBase + CPU_LAPIC_ID)]->CurrentThread;
    return Thread;
}

UINT64 KERNELAPI KeGetCurrentProcessId(){
    return KeGetCurrentProcess()->ProcessId;
}
UINT64 KERNELAPI KeGetCurrentThreadId(){
    return KeGetCurrentThread()->ThreadId;
}


RFTHREAD KERNELAPI KeGetProcessorIdleThread(UINT64 ProcessorId) {
    if (ProcessorId >= Pmgrt.NumProcessors || !CpuManagementTable[ProcessorId]->Initialized) return NULL;
    RFTHREAD Thread = CpuManagementTable[ProcessorId]->SystemIdleThread;
    return Thread;
}
// Roundability (Division size) for e.g (0.00 = 100 of Roundability, 0.0 = 10)
#define CALCULATE_ROUNDED_PERCENT(Slice, Source, Roundability) ((double)(((UINT64)Source * (UINT64)Roundability) / (UINT64)Slice) / (double)Roundability)

double KERNELAPI GetThreadCpuTime(RFTHREAD Thread) {
    // #ifdef ___KERNEL_DEBUG___
    //     DebugWrite("GetThreadCpuTime()");
    // #endif
    // while (Pmgrt.CpuTimeCalculation);
    // // Check to avoid Divided by 0 exceptions and SIMD Floating Point Exceptions
    // if (!Thread->CpuTime || !Pmgrt.EstimatedCpuTime) return 0;

    // return CALCULATE_ROUNDED_PERCENT(Thread->CpuTime, Pmgrt.EstimatedCpuTime, 1000);
    return 0;
}

double KERNELAPI GetIdleCpuTime() {
    // #ifdef ___KERNEL_DEBUG___
    //     DebugWrite("GetIdleCpuTime()");
    // #endif
    // while (Pmgrt.CpuTimeCalculation);
    // // Check to avoid Divided by 0 exceptions and SIMD Floating Point Exceptions
    // if (!Pmgrt.EstimatedIdleCpuTime) return 0;
    // if (!Pmgrt.EstimatedCpuTime) return 100;

    // return CALCULATE_ROUNDED_PERCENT(Pmgrt.EstimatedIdleCpuTime, Pmgrt.EstimatedCpuTime, 1000);
    return 0;
}
double KERNELAPI GetTotalCpuTime() {
    // #ifdef ___KERNEL_DEBUG___
    //     DebugWrite("GetTotalCpuTime()");
    // #endif
    // while (Pmgrt.CpuTimeCalculation);
    // // Check to avoid Divided by 0 exceptions and SIMD Floating Point Exceptions
    // if (!Pmgrt.EstimatedIdleCpuTime) return 100;
    // if (!Pmgrt.EstimatedCpuTime) return 0;

    // return 100 - CALCULATE_ROUNDED_PERCENT(Pmgrt.EstimatedIdleCpuTime, Pmgrt.EstimatedCpuTime, 1000);
    return 0;
}
double KERNELAPI GetProcessorIdleTime(UINT64 ProcessorId) {

    RFTHREAD ProcessorIdleThread = KeGetProcessorIdleThread(ProcessorId);
    if (!ProcessorIdleThread) return 0;
    return GetThreadCpuTime(ProcessorIdleThread);
}
double KERNELAPI GetProcessorTotalTime(UINT64 ProcessorId){
    // if (ProcessorId >= Pmgrt.NumProcessors || !CpuManagementTable[ProcessorId]->Initialized) return 0;
    // while (Pmgrt.CpuTimeCalculation);
    // // Check to avoid Divided by 0 exceptions and SIMD Floating Point Exceptions
    // if (!((RFTHREAD)CpuManagementTable[ProcessorId]->IdleThread)->CpuTime || !Pmgrt.EstimatedCpuTime) return 0;
    // return CALCULATE_ROUNDED_PERCENT(((RFTHREAD)CpuManagementTable[ProcessorId]->IdleThread)->CpuTime, CpuManagementTable[ProcessorId]->EstimatedCpuTime, 1000);
    return 0;
}

void KERNELAPI TaskSchedulerEnable() {
    Pmgrt.SchedulerEnable = TRUE;
}
void KERNELAPI TaskSchedulerDisable() {
    Pmgrt.SchedulerEnable = FALSE;
}

// Can be only set by the current thread
BOOL KERNELAPI IoWait() {
    KeGetCurrentThread()->State |= THS_IOWAIT;
    __Schedule();
    return TRUE;
}
BOOL KERNELAPI IoFinish(RFTHREAD Thread) {
    if (!Thread) return FALSE;
    Thread->State |= THS_IOPIN; // set IO_PIN Flag to remove IO_WAIT By task scheduler
    return TRUE;
}