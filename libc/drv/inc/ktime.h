// DLL : oskrnlx64.exe

#pragma once
#include <krnlapi.h>
// Declared in the kernel (OSKRNLX64.exe) : ACPI/hpet.h
UINT64 KERNELAPI GetHighPerformanceTimerFrequency();
UINT64 KERNELAPI GetHighPrecisionTimeSinceBoot();
// Declared in the kernel (OSKRNLX64.exe) : timedata/rtc.h