#include <Management/lock.h>
#include <mem.h>
#include <CPU/process.h>
#include <CPU/cpu.h>
BOOL KERNELAPI CreateLockObject(PCRITICAL_LOCK Lock) {
	if(!Lock) return FALSE;
	SZeroMemory(Lock);
	return TRUE;
}

void KERNELAPI EnterLockArea(PCRITICAL_LOCK Lock) {
	__cli();// Make sure that the procedure gets enough CPU Time

	UINT64 AwaitLockIndex = Lock->ScheduleCount;
	Lock->ScheduleCount++;
	__sti();
	while (Lock->LockIndex != AwaitLockIndex);
}

void KERNELAPI ExitLockArea(PCRITICAL_LOCK Lock) {
	__cli();// Make sure that the procedure gets enough CPU Time
	Lock->ScheduleCount--;
	if (!Lock->ScheduleCount)
		Lock->LockIndex = 0;
	else
		Lock->LockIndex++;

	__sti();
}