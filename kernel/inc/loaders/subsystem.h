#pragma once
#include <CPU/process.h>

#define GLOBAL_SUBSYSTEM_LOAD_ADDRESS 0x10000
#define GLOBAL_VIRTUAL_PMGRT 0x20000
// used for applications with unknown subsystem or threads
int ThreadWrapperInit(HTHREAD Thread, void* EntryPoint);
extern void KernelThreadWrapper();
extern void UserThreadWrapper();
