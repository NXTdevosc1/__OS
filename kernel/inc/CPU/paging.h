#pragma once
#include <stdint.h>
#include <CPU/paging_defs.h>
#include <CPU/process_defs.h>
#define SYS_RESTORE_CR3 __setCR3((UINT64)GetCurrentProcess()->PageMap)
//uint8_t set : if set then it will set the values of that page table otherwise it will clear them
int MapPhysicalPages(
    RFPAGEMAP PageMap,
    LPVOID VirtualAddress,
    LPVOID PhysicalAddress,
    UINT64 Count,
    UINT64 Flags
);
RFPAGEMAP CreatePageMap();
void* GetPhysAddr(RFPROCESS Process, const void* Ptr);