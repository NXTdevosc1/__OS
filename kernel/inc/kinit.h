#pragma once
#include <kernel.h>

void FirmwareControlRelease();
void KernelHeapInit();
void KernelPagingInitialize();

void InitFeatures();

// Wrapper for kernel relocate
extern void KernelRelocate();
// Actual Relocating Function
void __KernelRelocate();
// Jump to relocated kernel Function