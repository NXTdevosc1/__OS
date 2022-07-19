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
    25, //	Idle
    20, //	Low
    12, //	BelowNormal
    8 ,//	Normal
    4 ,//	AboveNormal
    3 ,//	High
    1 ,  //	TimeCritical
    0, // Idle Process (Balanced on heavy load)
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
        Process->ThreadHandles = CreateHandleTable();
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

RFPROCESS KERNELAPI CreateProcess(RFPROCESS ParentProcess, LPWSTR ProcessName, UINT64 Subsystem, UINT16 OperatingMode){

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

BOOL KERNELAPI SuspendThread(HTHREAD Thread) {
    if (!Thread) return FALSE;
    Thread->State &= ~(THS_ALIVE);
    //Pmgrt.TargetSuspensionThread = Thread;
    //IpiBroadcast(IPI_THREAD_SUSPEND, TRUE);
    return TRUE;
}
BOOL KERNELAPI ResumeThread(HTHREAD Thread) {
    if (!Thread) return FALSE;
    Thread->State |= THS_ALIVE;
    return TRUE;
}

BOOL SetThreadProcessor(HTHREAD Thread, UINT ProcessorId) {
    if (!Thread || ProcessorId >= Pmgrt.NumProcessors || !CpuManagementTable[ProcessorId]->Initialized)
        return FALSE;

    //SuspendThread(Thread);
    Thread->UniqueCpu = ProcessorId;
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

static inline void KERNELAPI AllocateThreadPriorityPtr(HTHREAD Thread, HTPTRLIST list, UINT16 Index){
    if(Thread->PriorityClassList){
        Thread->PriorityClassList->threads[Thread->PriorityClassIndex] = NULL;
        Thread->PriorityClassList->Count--;

        /*if (Thread->PriorityClassList->IndexMin == Thread->PriorityClassIndex) Thread->PriorityClassList->IndexMin++;
        if (Thread->PriorityClassList->IndexMax == Thread->PriorityClassIndex - 1) Thread->PriorityClassList->IndexMax--;*/


    }
    list->threads[Index] = Thread;
    Thread->PriorityClassList = list;
    Thread->PriorityClassIndex = Index;
    Thread->PriorityClassList->Count++;
    if (!(Thread->State & THS_REGISTRED))
    {
        Pmgrt.PriorityClassThreadCount[Thread->Process->PriorityClass - PRIORITY_CLASS_MIN]++;
        Thread->State |= THS_REGISTRED;
    }

}
HRESULT KERNELAPI SetThreadPriority(HTHREAD Thread, int Priority){
    if(!Thread || Priority < THREAD_PRIORITY_MIN || Priority > THREAD_PRIORITY_MAX || !Thread->Process->LpPriorityClass)
        return -1;

    RFPROCESS Process = Thread->Process;
    HTPTRLIST PtrList = Process->LpPriorityClass;
    BOOL _scheduler_state = Pmgrt.SchedulerEnable;
    Thread->ThreadPriority = Priority;
    Thread->PreemptionPriority = GlobalThreadPreemptionPriorities[Priority - THREAD_PRIORITY_MIN];
    Thread->RunAfter = Thread->PreemptionPriority;
    Thread->ThreadPriorityIndex = Priority - THREAD_PRIORITY_MIN;
    Pmgrt.SchedulerEnable = FALSE;
    for (;;) {
        if (PtrList->Count == PENTRIES_PER_LIST) goto NextList;
        else if (PtrList->IndexMin > 0) {
            AllocateThreadPriorityPtr(Thread, PtrList, PtrList->IndexMin - 1);
            if (PtrList->Index == PtrList->IndexMin)
                PtrList->Index--;


            PtrList->IndexMin--;
            Pmgrt.SchedulerEnable = _scheduler_state;
            return SUCCESS;
        }
        else if(PtrList->IndexMax < PENTRIES_PER_LIST){
            AllocateThreadPriorityPtr(Thread, PtrList, PtrList->IndexMax);
            PtrList->IndexMax++;
            Pmgrt.SchedulerEnable = _scheduler_state;
            return SUCCESS;
        } else {
            for (UINT16 i = 0; i < PENTRIES_PER_LIST; i++) {
                if(!PtrList->threads[i]){
                    AllocateThreadPriorityPtr(Thread, PtrList, i);
                    Pmgrt.SchedulerEnable = _scheduler_state;

                    return SUCCESS;
                }
            }
        }
        NextList:
        if(!PtrList->Next) break;
        PtrList = PtrList->Next;
    }

    PtrList->Next = kmalloc(sizeof(*PtrList));
    if(!PtrList->Next) SET_SOD_PROCESS_MANAGEMENT;
    PtrList = PtrList->Next;
    SZeroMemory(PtrList);
    AllocateThreadPriorityPtr(Thread, PtrList, 0);
    PtrList->IndexMax++;
    Pmgrt.SchedulerEnable = _scheduler_state;
    return SUCCESS;
}
HRESULT KERNELAPI SetPriorityClass(RFPROCESS Process, int PriorityClass) {
    if (!Process || PriorityClass < PRIORITY_CLASS_MIN || PriorityClass > PRIORITY_CLASS_MAX) return -1;

    UINT64* LastListCount = NULL;
    if (Process->PriorityClass)
        LastListCount = &Pmgrt.PriorityClassThreadCount[Process->PriorityClass - PRIORITY_CLASS_MIN];


    Process->PriorityClass = PriorityClass;
    Process->LpPriorityClass = Pmgrt.LpInitialPriorityClasses[PriorityClass - PRIORITY_CLASS_MIN];

    UINT64* NewListCount = &Pmgrt.PriorityClassThreadCount[Process->PriorityClass - PRIORITY_CLASS_MIN];
    HANDLE_ITERATION_STRUCTURE Iteration = { 0 };
    HANDLE Handle = NULL;
    if (!StartHandleIteration(Process->ThreadHandles, &Iteration))  SET_SOD_PROCESS_MANAGEMENT;
    while ((Handle = GetNextHandle(&Iteration))) {
        if (Handle->DataType == HANDLE_THREAD) {
            HTHREAD thread = Handle->Data;
            if (thread->ThreadPriority && LastListCount) {
                *LastListCount -= 1;
            }
            thread->State &= ~(THS_REGISTRED);
            SetThreadPriority(thread, thread->ThreadPriority);
        }
    }

    EndHandleIteration(&Iteration);

    return SUCCESS;
}

static inline void KERNELAPI SetupThreadRegisters(HTHREAD thread) {
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

static inline HTHREAD KERNELAPI AllocateThread(RFPROCESS Process, UINT64* LpThreadId) {
    HTHREADLIST ThreadList = &Pmgrt.ThreadList;
    UINT64 ListIndex = 0;
    HTHREAD thread = NULL;
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



HTHREAD KERNELAPI CreateThread(RFPROCESS Process, UINT64 StackSize, THREAD_START_ROUTINE StartAddress, UINT64 Flags, UINT64* LpThreadId, UINT8 NumParameters, ...){ // parameters
    if(!Process || !Process->Set || !Process->PriorityClass) return NULL;

    if (!StackSize) StackSize = THREAD_DEFAULT_STACK_SIZE;

    if (StackSize % 0x1000) StackSize += 0x1000 - (StackSize % 0x1000);

    HTHREAD Thread = AllocateThread(Process, LpThreadId);
    Thread->Process = Process;
    UINT64 ThreadProcessor = Pmgrt.NextThreadProcessor;
    
     #ifdef  ___KERNEL_DEBUG___
	DebugWrite("Specifiying Thread Processor.");
#endif //  ___KERNEL_DEBUG___
    // for(;;){
    //     if(!Pmgrt.NumProcessors) break;
    //     for (UINT64 i = ThreadProcessor; i < Pmgrt.NumProcessors; i++) {
    //         if(!CpuManagementTable[i]) continue;
    //         if (CpuManagementTable[i]->Initialized) {
    //             ThreadProcessor = i;
    //             goto ExitProcessorChoiceLoop;
    //         }
    //     }
    //     // if(!ThreadProcessor) break; // System Not Initialized Yet
    //     ThreadProcessor = 0;
    // }

    ExitProcessorChoiceLoop:
    
    
    Thread->UniqueCpu = ThreadProcessor;
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
    if (!Process->NumThreads) { // Setup Subsystem & Parameters are meaningless
        switch (Process->Subsystem) {
        case SUBSYSTEM_GUI:
        {
            SetThreadPriority(Thread, THREAD_PRIORITY_ABOVE_NORMAL);

            break;
        }
        case SUBSYSTEM_CONSOLE:
        {
            SetThreadPriority(Thread, THREAD_PRIORITY_NORMAL);

            break;
        }
        case SUBSYSTEM_NATIVE:
        {
            SetThreadPriority(Thread, THREAD_PRIORITY_BELOW_NORMAL);

            break;
        }
        default: {
            SetThreadPriority(Thread, THREAD_PRIORITY_BELOW_NORMAL);

            break;
        }
        }
    }
    else {
        // Pass parameters to the thread's start function
        SetThreadPriority(Thread, THREAD_PRIORITY_NORMAL);

    }

    
    if (!OpenHandle(Process->ThreadHandles, Thread, 0, HANDLE_THREAD, Thread, NULL)) SET_SOD_PROCESS_MANAGEMENT;
    
    if(!(Flags & THREAD_CREATE_SUSPEND)){
        ResumeThread(Thread);
    }
    ReducePriorityCountDown--;
    if (!ReducePriorityCountDown) {
        GlobalThreadPreemptionPriorities[PRIORITY_CLASS_INDEX_IDLE] += 5;
        GlobalThreadPreemptionPriorities[PRIORITY_CLASS_INDEX_LOW] += 5;
        GlobalThreadPreemptionPriorities[PRIORITY_CLASS_INDEX_BELOW_NORMAL] += 3;
        GlobalThreadPreemptionPriorities[PRIORITY_CLASS_INDEX_NORMAL] += 3;
        GlobalThreadPreemptionPriorities[PRIORITY_CLASS_INDEX_ABOVE_NORMAL] += 2;
        GlobalThreadPreemptionPriorities[PRIORITY_CLASS_INDEX_HIGH] += 1;
        //Realtime Does not increments. GlobalThreadPreemptionPriorities[PRIORITY_CLASS_INDEX_REALTIME] += 1;

        ReducePriorityCountDown = ReducePriorityAfter;
    }

    Thread->TimeSlice = 0; // run at 3 clocks per thread switch

    return Thread;

}


int KERNELAPI TerminateCurrentProcess(int exit_code){
    return TerminateProcess(GetCurrentProcess(), exit_code);
}
int KERNELAPI TerminateCurrentThread(int exit_code){
    return TerminateThread(GetCurrentThread(), exit_code);
}
int KERNELAPI TerminateProcess(RFPROCESS process, int ExitCode){
    return 0;
}
int KERNELAPI TerminateThread(HTHREAD thread, int ExitCode){
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


RFPROCESS KERNELAPI GetCurrentProcess(){
    if (!Pmgrt.SystemInitialized) return kproc;
    // *(UINT32*)(LAPIC_ADDRESS + LAPIC_TIMER_INITIAL_COUNT) = ApicTimerBaseQuantum / 3;
    // __cli();
    RFPROCESS Process = ((HTHREAD)(CpuManagementTable[GetCurrentProcessorId()]->Thread))->Process;
    // __sti();
    return Process;
}


HTHREAD KERNELAPI GetCurrentThread(){
    if (!Pmgrt.SystemInitialized) return kproc->StartupThread;

    // *(UINT32*)(LAPIC_ADDRESS + LAPIC_TIMER_INITIAL_COUNT) = ApicTimerBaseQuantum / 3;
    // __cli();
    HTHREAD Thread = CpuManagementTable[GetCurrentProcessorId()]->Thread;
    // __sti();
    return Thread;
}

UINT64 KERNELAPI GetCurrentProcessId(){
    return GetCurrentProcess()->ProcessId;
}
UINT64 KERNELAPI GetCurrentThreadId(){
    return GetCurrentThread()->ThreadId;
}


HTHREAD KERNELAPI GetProcessorIdleThread(UINT64 ProcessorId) {
    if (ProcessorId >= Pmgrt.NumProcessors || !CpuManagementTable[ProcessorId]->Initialized) return NULL;
    HTHREAD Thread = CpuManagementTable[ProcessorId]->IdleThread;
    return Thread;
}
// Roundability (Division size) for e.g (0.00 = 100 of Roundability, 0.0 = 10)
#define CALCULATE_ROUNDED_PERCENT(Slice, Source, Roundability) ((double)(((UINT64)Source * (UINT64)Roundability) / (UINT64)Slice) / (double)Roundability)

double KERNELAPI GetThreadCpuTime(HTHREAD Thread) {
    #ifdef ___KERNEL_DEBUG___
        DebugWrite("GetThreadCpuTime()");
    #endif
    while (Pmgrt.CpuTimeCalculation);
    // Check to avoid Divided by 0 exceptions and SIMD Floating Point Exceptions
    if (!Thread->CpuTime || !Pmgrt.EstimatedCpuTime) return 0;

    return CALCULATE_ROUNDED_PERCENT(Thread->CpuTime, Pmgrt.EstimatedCpuTime, 1000);
}

double KERNELAPI GetIdleCpuTime() {
    #ifdef ___KERNEL_DEBUG___
        DebugWrite("GetIdleCpuTime()");
    #endif
    while (Pmgrt.CpuTimeCalculation);
    // Check to avoid Divided by 0 exceptions and SIMD Floating Point Exceptions
    if (!Pmgrt.EstimatedIdleCpuTime) return 0;
    if (!Pmgrt.EstimatedCpuTime) return 100;

    return CALCULATE_ROUNDED_PERCENT(Pmgrt.EstimatedIdleCpuTime, Pmgrt.EstimatedCpuTime, 1000);
}
double KERNELAPI GetTotalCpuTime() {
    #ifdef ___KERNEL_DEBUG___
        DebugWrite("GetTotalCpuTime()");
    #endif
    while (Pmgrt.CpuTimeCalculation);
    // Check to avoid Divided by 0 exceptions and SIMD Floating Point Exceptions
    if (!Pmgrt.EstimatedIdleCpuTime) return 100;
    if (!Pmgrt.EstimatedCpuTime) return 0;

    return 100 - CALCULATE_ROUNDED_PERCENT(Pmgrt.EstimatedIdleCpuTime, Pmgrt.EstimatedCpuTime, 1000);
}
double KERNELAPI GetProcessorIdleTime(UINT64 ProcessorId) {

    HTHREAD ProcessorIdleThread = GetProcessorIdleThread(ProcessorId);
    if (!ProcessorIdleThread) return 0;
    return GetThreadCpuTime(ProcessorIdleThread);
}
double KERNELAPI GetProcessorTotalTime(UINT64 ProcessorId){
    if (ProcessorId >= Pmgrt.NumProcessors || !CpuManagementTable[ProcessorId]->Initialized) return 0;
    while (Pmgrt.CpuTimeCalculation);
    // Check to avoid Divided by 0 exceptions and SIMD Floating Point Exceptions
    if (!((HTHREAD)CpuManagementTable[ProcessorId]->IdleThread)->CpuTime || !Pmgrt.EstimatedCpuTime) return 0;
    return CALCULATE_ROUNDED_PERCENT(((HTHREAD)CpuManagementTable[ProcessorId]->IdleThread)->CpuTime, CpuManagementTable[ProcessorId]->EstimatedCpuTime, 1000);
}

void KERNELAPI TaskSchedulerEnable() {
    Pmgrt.SchedulerEnable = TRUE;
}
void KERNELAPI TaskSchedulerDisable() {
    Pmgrt.SchedulerEnable = FALSE;
}

// Can be only set by the current thread
BOOL KERNELAPI IoWait() {
    GetCurrentThread()->State |= THS_IOWAIT;
    __Schedule();
    return TRUE;
}
BOOL KERNELAPI IoFinish(HTHREAD Thread) {
    if (!Thread) return FALSE;
    Thread->State |= THS_IOPIN; // set IO_PIN Flag to remove IO_WAIT By task scheduler
    return TRUE;
}