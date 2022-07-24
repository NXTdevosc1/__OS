#include <interrupt_manager/interrupts.h>
#include <CPU/cpu.h>
#include <CPU/process.h>
#include <preos_renderer.h>
#include <cstr.h>
void INTH_IPI(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame) {
	__cli();

	UINT ProcessorId = GetCurrentProcessorId();
	UINT Command = 0;
	BOOL BroadCast = FALSE;
	if (Pmgrt.BroadCastIpi) {
		Command = Pmgrt.BroadCastIpiCommand;
		BroadCast = TRUE;
		Pmgrt.BroadCastNumProcessors++;
	}
	else {
		Command = CpuManagementTable[ProcessorId]->IpiCommand;
	}

	switch (Command)
	{
	case IPI_SHUTDOWN:
	{
		for (;;) __hlt();
		break;
	}
	case IPI_THREAD_SUSPEND:
	{
		// if (CpuManagementTable[ProcessorId]->Thread == Pmgrt.TargetSuspensionThread) {
		// 	CpuManagementTable[ProcessorId]->TaskSchedulerData.RemainingCpuTime = 0;
		// 	__sti();
		// 	__hlt(); // Wait until next task switch
		// }
		break;
	}
	default:
		break;
	}
	

	if(!BroadCast) {
		CpuManagementTable[ProcessorId]->IpiCommand = 0;
		__BitRelease(&CpuManagementTable[ProcessorId]->IpiCommandControl, 0);
	}
	__sti();
}