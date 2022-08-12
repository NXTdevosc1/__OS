/*IMPORTED in : kernelruntime.dll directly linked to the kernel*/
#pragma once
#include <kerneltypes.h>


// Declared in the kernel (OSKRNLX64.exe) : ACPI/hpet.h
UINT64 GetHighPerformanceTimerFrequency();
UINT64 GetHighPrecisionTimeSinceBoot();
// Declared in the kernel (OSKRNLX64.exe) : timedata/rtc.h