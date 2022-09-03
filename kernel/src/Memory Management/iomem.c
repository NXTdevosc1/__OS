#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <kernel.h>
#include <CPU/cpu.h>
typedef struct _IO_BITMAP IO_BITMAP;

#define IO_BITMAP_SIZE 0x1FF

// Fit in 1 Page
typedef struct _IO_BITMAP {
    UINT64 IoBitmap[IO_BITMAP_SIZE];
    IO_BITMAP* Next;
} IO_BITMAP;

IO_BITMAP IoBitmap = {0};

BOOL IoMemMutex = FALSE;

LPVOID KEXPORT KERNELAPI AllocateIoMemory(_IN LPVOID PhysicalAddress, _IN UINT64 NumPages, _IN UINT Flags) {
    if(!NumPages || MemoryManagementTable.IoMemorySize + (NumPages << 12) > (SYSTEM_SPACE48_RAM - SYSTEM_SPACE48_IO)) return NULL;
    __SpinLockSyncBitTestAndSet(&IoMemMutex, 0);
    IO_BITMAP* Bmp = &IoBitmap;
    char* MajorVirtualAddress = (char*)SYSTEM_SPACE48_IO;
    IO_BITMAP* StartBmp = NULL;
    int StartIndex = 0, StartBit = 0;
    UINT64 Pages = 0;
    void* VirtualStart = NULL;
    for(;;MajorVirtualAddress += (0xFFF << 12)) {
        char* Vaddr = MajorVirtualAddress;
        for(register UINT k = 0;k<IO_BITMAP_SIZE;k++) {
            register UINT BitShift = 1;
            register UINT64 BitMask = Bmp->IoBitmap[k];
            for(register UINT i = 0;i<64;i++, Vaddr+=0x1000, BitShift <<= 1) {
                if(BitMask & BitShift) {
                    if(StartBmp) {
                        StartBmp = NULL;
                        Pages = 0;
                    } else {
                        if(!StartBmp) {
                            StartBmp = Bmp;
                            StartIndex = k;
                            StartBit = i;
                            VirtualStart = Vaddr;
                        }
                        Pages++;
                        if(Pages == NumPages) {
                            // Fill the bitmap
                            register UINT64 x = StartBit;
                            register UINT64 v = StartIndex;
                            register UINT64* f = NULL;
                            register UINT64 Value = 0;
                            // for(;;) {
                            //     for(;v < IO_BITMAP_SIZE;v++, f++) {
                            //         f = &StartBmp->IoBitmap[v];
                            //         Value = *f;
                            //         for(;x < 64;x++) {
                            //             if(!Pages) goto Ret;
                            //             Value |= (1 << x);
                            //             Pages--;
                            //         }
                            //         *f = Value;
                            //     }
                            //     Bmp = Bmp->Next;
                            // }

                            Ret:
                            *f = Value;
                            __BitRelease(&IoMemMutex, 0);
                            UINT MapFlags = PM_MAP | PM_NX;
                            if(Flags & IO_MEMORY_WRITE_THROUGH) MapFlags |= PM_WRITE_THROUGH;
                            if(Flags & IO_MEMORY_CACHE_DISABLED) MapFlags |= PM_CACHE_DISABLE;

                            MapPhysicalPages(kproc->PageMap, VirtualStart, PhysicalAddress, NumPages, MapFlags);
                            return VirtualStart;
                        }
                    }
                }
            }
        }
        if(!Bmp) {
            Bmp->Next = _SIMD_AllocatePhysicalPage(MemoryManagementTable.PageBitmap, MemoryManagementTable.NumBytesPageBitmap, MemoryManagementTable.PageArray);
            if(!Bmp->Next) SET_SOD_MEMORY_MANAGEMENT;
            ZeroMemory(Bmp->Next, sizeof(IO_BITMAP));
        }
        Bmp = Bmp->Next;
    }
}
BOOL KEXPORT KERNELAPI FreeIoMemory(_IN LPVOID IoMemory) {
    return FALSE;
}