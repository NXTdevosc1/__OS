#include <Management/debug.h>
#include <interrupt_manager/SOD.h>
#include <stdlib.h>
#include <mem.h>
#include <CPU/cpu.h>
#define SET_SOD_DEBUG_ERROR SOD(0, "KERNEL DEBUG ERROR")


__declspec(allocate("_KRNLDBG")) __declspec(align(0x1000)) DEBUG_TABLE KernelDebugTable = { 0 };

void DebugWrite(const char* Text) {
	size_t len = strlen(Text);
	if (len > DEBUG_MAX_LOG_LENGTH) SET_SOD_DEBUG_ERROR;
	__SpinLockSyncBitTestAndSet(&KernelDebugTable.ControlBit, 0);

	memcpy(KernelDebugTable.LogEntries[KernelDebugTable.LogAllocateIndex], Text, len);
	KernelDebugTable.LogEntries[KernelDebugTable.LogAllocateIndex][len] = 0;

	KernelDebugTable.LogAllocateIndex++;
	KernelDebugTable.TotalLogCount++;
	if (KernelDebugTable.LogAllocateIndex == DEBUG_MAX_LOG_ENTRIES) KernelDebugTable.LogAllocateIndex = 0;
	__BitRelease(&KernelDebugTable.ControlBit, 0);

}
char* DebugRead(UINT Index) {
	if (Index > DEBUG_MAX_INDEX) SET_SOD_DEBUG_ERROR;
	__SpinLockSyncBitTestAndSet(&KernelDebugTable.ControlBit, 0);
	UINT64 StringIndex = 0;
	if (!KernelDebugTable.LogAllocateIndex) StringIndex = DEBUG_MAX_INDEX;
	else StringIndex = KernelDebugTable.LogAllocateIndex - 1;

	for (UINT i = 0; i < Index; i++) {
		if (!StringIndex) StringIndex = DEBUG_MAX_INDEX;
		else StringIndex--;
	}
	__BitRelease(&KernelDebugTable.ControlBit, 0);
	return KernelDebugTable.LogEntries[StringIndex];
}



