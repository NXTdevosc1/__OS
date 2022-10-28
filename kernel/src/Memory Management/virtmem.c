#include <CPU/process_defs.h>
#include <MemoryManagement.h>
#include <intrin.h>
#include <CPU/paging.h>
#include <kernel.h>
#include <CPU/cpu.h>
#include <math.h>
// Allocates in kernel mode RAM Space
void KEXPORT *KERNELAPI KeAllocateVirtualMemory(UINT64 NumBytes) {
    UINT64 NumPages = NumBytes >> 12;
    if(NumBytes & 0xFFF) NumPages++;
    MutexWait64(kproc->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_VIRTUAL_MEMORY);
    LPVOID Vmem = VirtualFindAvailableMemory(kproc, (LPVOID)((char*)SystemSpaceBase + SYSTEM_SPACE48_RAM), (LPVOID)((char*)SystemSpaceBase + SYSTEM_SPACE48_MAX), NumPages);
    if(!Vmem) {
        MutexRelease64(kproc->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_VIRTUAL_MEMORY);
    }
    
    char* Off = Vmem;
    PAGE* Page = MemoryManagementTable.PageArray;
    UINT64* Bitmap = (UINT64*)MemoryManagementTable.PageBitmap;
    register const UINT64 Max = MemoryManagementTable.NumBytesPageBitmap >> 3;
    for(UINT64 i = 0;i<Max;i++, Page+=64) {
        register UINT64 _Bmp = *Bitmap;
        if(!_Bmp) {
            if(NumPages >= 64) {
                *Bitmap = (UINT64)-1;
                NumPages-=64;
                MapPhysicalPages(kproc->PageMap, Off, (LPVOID)(Page->PhysicalAddress & ~0xFFF), 64, PM_MAP);
                if(!NumPages) {
                    return Vmem;
                }
                Off += 0x40000;
            } else {
                *Bitmap = (UINT64)-1 >> (64 - NumPages);
                MapPhysicalPages(kproc->PageMap, Off, (LPVOID)(Page->PhysicalAddress & ~0xFFF), NumPages, PM_MAP);
                return Vmem;
            }
        } else {
            // Sort free pages with First/Last bit indexes
            unsigned long Index;
            _BitScanForward64(&Index, _Bmp);
            if(Index) {
                NumPages -= min(NumPages, Index);
                _Bmp |= (UINT64)-1 >> (64 - Index);
                *Bitmap = _Bmp;
                MapPhysicalPages(kproc->PageMap, Off, (Page[Index].PhysicalAddress & ~0xFFF), Index, PM_MAP);
                if(!NumPages) return Vmem;
                Off += (Index << 12);
            }
            if(_BitScanReverse64(&Index, _Bmp)) {
                NumPages -= min(NumPages, 64 - Index);
                if(NumPages < 64 - Index) Index = 64 - NumPages;
                _Bmp |= (UINT64)-1 << Index;
                *Bitmap = _Bmp;
                MapPhysicalPages(kproc->PageMap, Off, (Page[Index].PhysicalAddress & ~0xFFF), 64 - Index, PM_MAP);
                if(!NumPages) return Vmem;
            }

            // Find Bits in the middle

        }
            
    }

    MutexRelease64(kproc->ControlMutex0, PROCESS_MUTEX0_ALLOCATE_VIRTUAL_MEMORY);
    return NULL;
}

// Allocates in user mode space (Random area inside the low 128 TB)
