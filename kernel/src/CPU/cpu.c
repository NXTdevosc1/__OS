#include <CPU/cpu.h>
#include <interrupt_manager/idt.h>
#include <IO/utility.h>
#include <input/mouse.h>
#include <input/keyboard.h>
#include <interrupt_manager/SOD.h>
#include <acpi/madt.h>
#include <preos_renderer.h>
#include <cstr.h>
#include <MemoryManagement.h>
#include <sysentry/sysentry.h>
#include <stdlib.h>
#include <CPU/pcid.h>
#include <Management/debug.h>
#include <smbios/smbios.h>
#include <cmos.h>
#include <IO/pit.h>
#include <acpi/hpet.h>
CPU_MANAGEMENT_TABLE** CpuManagementTable = NULL;
KERNELSTATUS CpuBootStatus = 0;

BOOL __CPU_MGMT_TBL_SETUP__ = FALSE;
UINT64 ApicTimerClockQuantum = 0;
// 128 Bit integer used to count timer clocks to give precise times on timing functions such as sleep
__declspec(align(0x1000)) UINT64 ApicTimerClockCounter = 0; /*CACHE Will be disabled on this page*/
UINT64 __DMP[0xFFF] = {0};
UINT64 TimerClocksPerSecond = 0; // will get divided until to the right value (in microseconds)
UINT32 TimerIncrementerCpuId = 0;
void InitProcessorDescriptors(void** CpuBuffer, UINT64* CpuBufferSize){
	UINT64 _CpuBufferNumPages = 20 + TSS_IST1_NUMPAGES + TSS_IST2_NUMPAGES + TSS_IST3_NUMPAGES + SYSENTRY_STACK_NUMPAGES;
	void* _CpuBuffer = kpalloc(_CpuBufferNumPages);
	if (!_CpuBuffer) SET_SOD_MEMORY_MANAGEMENT;
	ZeroMemory(_CpuBuffer, _CpuBufferNumPages << 12);
	GlobalCpuDescriptorsInitialize(_CpuBuffer);
	GlobalInterruptDescriptorLoad();
	InitSysEntry(_CpuBuffer);

	*CpuBuffer = _CpuBuffer;
	*CpuBufferSize = _CpuBufferNumPages << 12;

	// Checking & Enabling PCID
	#ifdef ___KERNEL_DEBUG___
		DebugWrite("Checking & Enabling Process Context ID (TLB Cache Extension)");
	#endif
	if ((ProcessContextIdSupported = CheckProcessContextIdEnable())) {
		#ifdef ___KERNEL_DEBUG___
		DebugWrite("Process Context ID (TLB Cache Extension) Is Supported.");
	#endif
		ProcessContextIdEnabled = TRUE;
		// CheckInvPcidSupport();
	}
	else {
		#ifdef ___KERNEL_DEBUG___
		DebugWrite("Process Context ID (TLB Cache Extension) Is Not Supported.");
	#endif
		ProcessContextIdSupported = FALSE; // Not all processors support PCID
		ProcessContextIdEnabled = FALSE;
	}

	// // Setup SSE

	// unsigned int MXCSR = __stmxcsr();
	// __ldmxcsr(MXCSR | (0x8000 /*Flush to zero on error*/) | (0x2000 /*towards negative infinity rounding mode*/));

}

#define MILLISECONDS_PER_SECOND 1000
#define MICROSECONDS_PER_SECOND 1000000
#define MICROSECONDS_PER_MILLISECOND 1000




void CpuSetupManagementTable(UINT64 CpuCount) {
	if (__CPU_MGMT_TBL_SETUP__) return;

	CpuManagementTable = AllocatePoolEx(NULL, (CpuCount << 3) + 0x1000, 0x1000, NULL, 0);
	if (!CpuManagementTable) SET_SOD_MEMORY_MANAGEMENT;
	ZeroMemory(CpuManagementTable, (CpuCount << 3) + 0x1000);
	UINT64 CurrentProcessor = GetCurrentProcessorId();
	for (UINT64 i = 0; i < CpuCount; i++) {
		CpuManagementTable[i] = kpalloc(CPU_MGMT_NUM_PAGES);
		if (!CpuManagementTable[i]) SET_SOD_MEMORY_MANAGEMENT;
		SZeroMemory(CpuManagementTable[i]);
		RFTHREAD_WAITING_QUEUE WaitingQueue = AllocatePoolEx(kproc->StartupThread, sizeof(THREAD_WAITING_QUEUE) * NUM_PRIORITY_CLASSES, 0x1000, NULL, 0);
		ZeroMemory(WaitingQueue, sizeof(THREAD_WAITING_QUEUE) * NUM_PRIORITY_CLASSES);
		if(i == CurrentProcessor){
			CpuManagementTable[i]->Initialized = TRUE;
		}
		CpuManagementTable[i]->ProcessorId = i;
		for (UINT64 c = 0; c < NUM_PRIORITY_CLASSES; c++) {
			CpuManagementTable[i]->ThreadQueues[c] = WaitingQueue + c;
			// CpuManagementTable[i]->HighestPriorityThread[c] = (UINT64)-1;
		}
		// CpuManagementTable[i]->TaskSchedulerData.cr3 = KeGlobalCR3;
		

		CpuManagementTable[i]->SystemIdleThread = KeCreateThread(IdleProcess, 0x1000, IdleThread, THREAD_CREATE_SYSTEM_IDLE | THREAD_CREATE_MANUAL, NULL);
		CpuManagementTable[i]->SystemInterruptsThread = KeCreateThread(SystemInterruptsProcess, 0x10000, NULL, THREAD_CREATE_SYSTEM_IDLE | THREAD_CREATE_MANUAL, NULL);
		if (!CpuManagementTable[i]->SystemIdleThread || !CpuManagementTable[i]->SystemInterruptsThread) SET_SOD_PROCESS_MANAGEMENT;
		

		// Setup Idle Thread
		HTHREAD hIdleThread = CpuManagementTable[i]->SystemIdleThread;
		CpuManagementTable[i]->CurrentThread = hIdleThread;
		hIdleThread->ProcessorId = i; // Make thread only runnable on this cpu
		hIdleThread->SchedulingQuantum = 0;
		hIdleThread->TimeBurst = 0;


		// Setup Interrupts Thread
		HTHREAD InterruptsThread = CpuManagementTable[i]->SystemInterruptsThread;
		InterruptsThread->ProcessorId = i;
		InterruptsThread->Registers.rflags = 0; // Interrupts disabled
		InterruptsThread->TimeBurst = 0;
	}
	__CPU_MGMT_TBL_SETUP__ = TRUE;
}


void DeclareCpuHalt() {
	__hlt();
}
__declspec(align(0x1000)) USER_SYSTEM_CONFIG GlobalUserSystemConfig = { 0 };

extern void SetupCPU() {
	EnableExtendedStates();
	UINT32 ProcessorId = GetCurrentProcessorId();
	InitProcessorDescriptors(&CpuManagementTable[ProcessorId]->CpuBuffer, &CpuManagementTable[ProcessorId]->CpuBufferSize);
	EnableApic();
	SetupLocalApicTimer();
	SystemDebugPrint(L"Processor#%d ON", ProcessorId);
	CpuManagementTable[ProcessorId]->Initialized = TRUE;
	// CpuManagementTable[ProcessorId]->CpuId = ProcessorId;
	CpuBootStatus = 1; // Declare successful CPU Boot
	__wbinvd();
	__pause();
	while(!Pmgrt.SystemInitialized) __pause();
	__sti();
	for (;;) {
		// __hlt();
	}

}



KERNELSTATUS InitializeApicCpu(UINT64 ApicId) {

	CpuBootStatus = 0;

	*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_ERROR_STATUS) = 0;
	*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_HIGH) = (*(uint32_t*)(LAPIC_ADDRESS + 0x310) & 0x00ffffff) | (ApicId << 24);
	*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) = (*(uint32_t*)(LAPIC_ADDRESS + 0x300) & 0xfff00000) | 0x00C500;



	while (*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) & (1 << 12)) {
		__pause();
	}



	*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_HIGH) = (*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_HIGH) & 0x00ffffff) | (ApicId << 24);
	*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) = (*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) & 0xfff00000) | 0x008500;

	while (*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) & (1 << 12)) {
		__pause();
	}

	for (uint8_t i = 0; i < 2; i++) {
		*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_ERROR_STATUS) = 0;
		*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_HIGH) = (*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_HIGH) & 0x00ffffff) | (ApicId << 24);
		*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) = (*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) & 0xfff0f800) | 0x000608;
		while (*(uint32_t*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) & (1 << 12)) {
			__pause();
		}
	}
	while(!(CpuManagementTable[ApicId]->Initialized)) {
		__pause();
	}

	if (CpuBootStatus == 2) return -1;

	return 0;
}

UINT64 ApicTimerBaseQuantum = 0;



void BroadcastInterrupt(char InterruptNumber, BOOL SelfSend) {
	UINT32 Entry = (UINT32)InterruptNumber | (5 << 8 /*Destination Mode : NMI*/);
	Entry |= (1 << 14);

	if (SelfSend) {
		Entry |= (2 << 18); // Send it to all processors and the current one
	}
	else {
		Entry |= (3 << 18); // Send to all processors without the current one
	}
	// set destination type (bits 18 - 19) to send to all processors


	// Destination Processor
	*(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) = Entry;
	while (*(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) & (1 << 12));

}

void SendProcessorInterrupt(UINT ApicId, char InterruptNumber) {
	// Delivery Status, Wait until interrupt ends
	UINT32 Entry = (UINT32)InterruptNumber | (5 << 8 /*Destination Mode : NMI*/);
	Entry |= (1 << 14);

	// set destination type (bits 18 - 19) to send to all processors
	
	
	// Destination Processor
	*(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_HIGH) = 
	( * (UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_HIGH) & 0x00ffffff)
		| (ApicId << 24);
	*(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) = Entry;
	while (*(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_INTERRUPT_COMMAND_REGISTER_LOW) & (1 << 12));

}

void IpiSend(UINT ApicId, UINT Command) {
	if (CpuManagementTable[ApicId] && CpuManagementTable[ApicId]->Initialized) {
		__SpinLockSyncBitTestAndSet(&CpuManagementTable[ApicId]->IpiCommandControl, 0); // Unsetted by processor
		CpuManagementTable[ApicId]->IpiCommand = Command;
		SendProcessorInterrupt(ApicId, IPI_INTVEC);
	}
}
void IpiBroadcast(UINT Command, BOOL SelfSend) {
	__SpinLockSyncBitTestAndSet(&Pmgrt.BroadCastIpi, 0);
	Pmgrt.BroadCastIpiCommand = Command;
	BroadcastInterrupt(IPI_INTVEC, SelfSend);
	//UINT TargetProcessorsCount = Pmgrt.NumProcessors - 1 + (SelfSend & 1); // if self send then the current processor is target
	//while (Pmgrt.BroadCastNumProcessors < 1);
	//Pmgrt.BroadCastNumProcessors = 0;
	__BitRelease(&Pmgrt.BroadCastIpi, 0);
}

extern void _TimeSliceCounter();

void LapicTimerSetupTSCDeadlineMode() {
	GP_clear_screen(0xffff0000);
	GP_draw_sf_text("CPU LAPIC Deadline Mode Support", 0xffffff, 20, 20);
	
	*(UINT32*)(LAPIC_ADDRESS + LAPIC_TIMER_LVT) = 0x20 | (LAPIC_TIMER_TSC_DEADLINE_MODE); // IRQ 0 | ONE SHOT MODE
	ApicTimerBaseQuantum = ((UINT64)1 << 63) | 0x18000;
	__pause();
	GP_draw_sf_text(to_hstring32(__rdtsc()), 0xffffff, 20, 40);
	*(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_END_OF_INTERRUPT) = 0;

	__WriteMsr(IA32_TSC_DEADLINE_MSR, (unsigned int)ApicTimerBaseQuantum, 0);

	GP_draw_sf_text(to_hstring32(__rdtsc()), 0xffffff, 20, 60);


	while (1);
}

static UINT64 CpuBusSpeed = 0;

void SetupLocalApicTimer(){
	
	__cli();
	CPUID_INFO CpuInfo = {0};
	__cpuid(&CpuInfo, 1);
	// if (CpuInfo.ecx & (CPUID1_ECX_APICTMR_TSC_DEADLINE)) {
	// 	LapicTimerSetupTSCDeadlineMode();
	// }
	// else {
		// Setup LAPIC TIMER Periodic Mode
		*(UINT32*)(LAPIC_ADDRESS + LAPIC_TIMER_DIVISOR) = 0b11; // divide by 16
		
		if(!CpuBusSpeed) {
			// Find CPU BUS SPEED
			// Processor Info of the current Processor
			SMBIOS_PROCESSOR_INFORMATION_STRUCTURE* ProcessorInformation = NULL;
			UINT SmNum = SmbiosGetStructuresCount();
			
			for(UINT i = 0;i<SmNum;i++) {
				SMBIOS_STRUCTURE_HEADER* Sm = SmbiosGetStructure(i);
				if(!Sm) SOD(SOD_INITIALIZATION, "Cannot retreive CPU Information and Bus Speed");
				if(Sm->Type == SMBIOS_PROCESSOR_INFORMATION) {
					ProcessorInformation = (void*)Sm;
					CpuBusSpeed = ProcessorInformation->ExternalClock * 1000000;
					break;
				}
			}

		}
		if(!CpuBusSpeed) {
			// We will assum a 100MHz Bus Frequency
			CpuBusSpeed = 100000000;
			SystemDebugPrint(L"EXTERNAL_BUS_SPEED : %d", CpuBusSpeed);
		}
		UINT64 InitialCount = ((CpuBusSpeed) / 0x10 /*Divisor*/) / 0x800 /*Target clocks per second*/;

		*(UINT32*)(LAPIC_ADDRESS + LAPIC_TIMER_LVT) = INT_APIC_TIMER | (LAPIC_TIMER_PERIODIC_MODE); 
		*(UINT32*)(LAPIC_ADDRESS + LAPIC_TIMER_INITIAL_COUNT) = InitialCount;
	
}




// Base Quantum = Timer Update Rate / s
void KERNELAPI Sleep(UINT64 Milliseconds){
	HTHREAD Thread = KeGetCurrentThread();
	Thread->SleepUntil[0] = GetHighPrecisionTimeSinceBoot() + GetHighPerformanceTimerFrequency() / MILLISECONDS_PER_SECOND * Milliseconds;
	Thread->State |= THS_SLEEP;
	Thread->RemainingClocks = 0; // Reset remaining clocks to force scheduler to not re-run this thread
	__Schedule();
}



void KERNELAPI MicroSleep(UINT64 Microseconds){
	HTHREAD Thread = KeGetCurrentThread();
	Thread->SleepUntil[0] = GetHighPrecisionTimeSinceBoot() + GetHighPerformanceTimerFrequency() / MICROSECONDS_PER_SECOND * Microseconds;
	Thread->State |= THS_SLEEP;
	Thread->RemainingClocks = 0;

	__Schedule();
}





void EnableApic() {
	SetLocalApicBase(LocalApicPhysicalAddress);
	
	*(UINT32*)((char*)LocalApicPhysicalAddress + CPU_LAPIC_LOGICAL_DESTINATION_REGISTER) = 0;
	*(UINT32*)((char*)LocalApicPhysicalAddress + CPU_LAPIC_DESTINATION_FORMAT_REGISTER) = 0xF << 28; // Flat Mode
	*(UINT32*)((char*)LocalApicPhysicalAddress + CPU_LAPIC_TASK_PRIORITY) = 0;

	*(UINT32*)((char*)LocalApicPhysicalAddress + CPU_LAPIC_SPURIOUS_INTERRUPT_VECTOR) = INT_APIC_SPURIOUS | 0x100 ; // Bit 12 (do not broadcast EOI), bit 8 : APIC Enable
	// *(UINT32*)((char*)LocalApicPhysicalAddress + CPU_LAPIC_LVT_THERMAL_SENSOR) = INT_TSR;
	// *(UINT32*)((char*)LocalApicPhysicalAddress + CPU_LAPIC_PERFORMANCE_MONITORING_COUNTERS) = INT_PMCR;
	// *(UINT32*)((char*)LocalApicPhysicalAddress + CPU_LAPIC_LVT_LINT0) = INT_LINT0;
	// *(UINT32*)((char*)LocalApicPhysicalAddress + CPU_LAPIC_LVT_LINT1) = INT_LINT1;
	*(UINT32*)((char*)LocalApicPhysicalAddress + CPU_LAPIC_LVT_ERR) = INT_LVT_ERROR;
	// *(UINT32*)((char*)LocalApicPhysicalAddress + CPU_LAPIC_LVT_CMCI) = INT_CMCI;

	


}
void SetLocalApicBase(void* _Lapic) {
	UINT32 eax = ((UINT64)_Lapic | IA32_APIC_BASE_MSR_ENABLE);
	UINT32 edx = (UINT64)_Lapic >> 32;
	__WriteMsr(IA32_APIC_BASE_MSR, eax, edx);
}
