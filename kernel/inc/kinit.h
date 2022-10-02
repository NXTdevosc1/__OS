#pragma once
#include <kernel.h>

void FirmwareControlRelease(void);
void KernelHeapInit(void);
void KernelPagingInitialize(void);

void InitFeatures(void);

// Wrapper for kernel relocate
extern void KernelRelocate(void);
// Actual Relocating Function
void __KernelRelocate(void);
// Jump to relocated kernel Function

void KeInitOptimizedComputing(void);