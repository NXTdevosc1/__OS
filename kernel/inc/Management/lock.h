#pragma once
#include <krnltypes.h>
typedef UINT64 LOCK_KEY;
typedef struct _CRITICAL_LOCK {
	UINT64 ScheduleCount;
	UINT64 LockIndex;
} CRITICAL_LOCK, *PCRITICAL_LOCK;

BOOL KERNELAPI CreateLockObject(PCRITICAL_LOCK Lock);
void KERNELAPI EnterLockArea(PCRITICAL_LOCK Lock);
void KERNELAPI ExitLockArea(PCRITICAL_LOCK Lock);