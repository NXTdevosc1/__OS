#include <MemoryManagement.h>
#include <CPU/cpu.h>
#include <interrupt_manager/SOD.h>
#include <math.h>

// RFMEMORY_SEGMENT AllocateMemorySegment(RFMEMORY_REGION_TABLE MemoryRegion) {
//     if(MemoryRegion->UnallocatedSegmentCache.CacheLineSize) {
//         RFMEMORY_SEGMENT Segment = _SIMD_FetchMemoryCacheLine(&MemoryRegion->UnallocatedSegmentCache);
//         if(Segment) return Segment;
//     }

//     if(MemoryRegion->TotalEntries == MemoryRegion->AllocatedEntries) {
//         if(__SyncBitTestAndSet(&MemoryRegion->AllocateListControl, 0)) {
//             MemoryRegion->LastListHead->NextListHead = AllocatePoolEx(kproc, sizeof(MEMORY_SEGMENT_LIST_HEAD), 0, ALLOCATE_POOL_NEW_LIST_HEAD);
//             SZeroMemory(MemoryRegion->LastListHead->NextListHead);
//             MemoryRegion->LastListHead = MemoryRegion->LastListHead->NextListHead;
//             RFMEMORY_SEGMENT Segment = _SIMD_FetchUnusedSegmentsUncached(MemoryRegion->LastListHead);
//             __BitRelease(&MemoryRegion->AllocateListControl, 0);
//             return Segment;
//         } else {
//             // Prevent a return NULL if the free list is not yet allocated
//             while(MemoryRegion->AllocateListControl, 0) __Schedule();
//         }
//     }
//     return _SIMD_FetchUnusedSegmentsUncached(&MemoryRegion->SegmentListHead);
// }

volatile long AllocateContiguousMutex = 0;

void* AllocateContiguousPages(RFPROCESS Process, UINT64 NumPages, UINT64 Flags) {
    
    volatile UINT64* Bmp = (UINT64*)MemoryManagementTable.PageBitmap;
    register PAGE* Page = MemoryManagementTable.PageArray;
    register UINT64 Max = MemoryManagementTable.NumBytesPageBitmap >> 3; // QWORD Size
    register UINT NextIndex = 0;
    unsigned long Index = 0;
    
    volatile UINT64* PagesStartBmp = NULL;
    UINT PagesStartIndex = 0;
    UINT Arrival = 0;
    void* PhysicalStart = NULL;

    while(_interlockedbittestandset(&AllocateContiguousMutex, 0)) _mm_pause();

    for(UINT64 i = 0;i<Max;i++, Bmp++, Page+=64) {
        NextIndex = 0;
        __int64 BitMap = *Bmp;
        while(_BitScanForward64(&Index, ~(UINT64)BitMap)) {
            _bittestandset64(&BitMap, Index);
            if(Index != NextIndex) Arrival = 0;

            if(!Arrival) {
                PagesStartBmp = Bmp;
                PagesStartIndex = Index;
                PhysicalStart = (void*)(((PAGE*)(Page + Index))->PageStruct.PhysicalPageNumber << 12);
            }
            Arrival++;
            if(Arrival == NumPages) {
                if(PagesStartBmp == Bmp) {
                    // Clear bits from StartIndex to Index

                    // Lowerhalf
                    UINT64 Num = 0;
                    Num = ((UINT64)-1 >> (64 - NumPages));
                    Num <<= PagesStartIndex;
                    *Bmp |= Num;
                    
                } else {
                    // Fill bitmap
                    if(PagesStartBmp < Bmp) {
                        // Clear bits from StartIndex to 63
                        *PagesStartBmp |= ((UINT64)-1 >> (63 - PagesStartIndex));
                        if((64 - PagesStartIndex) <= NumPages) {
                            NumPages -= (64 - PagesStartIndex);
                        } else NumPages = 0;
                        SystemDebugPrint(L"X(%x, %x)", ((UINT64)-1 >> (63 - PagesStartIndex)), *PagesStartBmp);
                        PagesStartIndex = 0;
                        PagesStartBmp++;
                    }
                    while(PagesStartBmp < Bmp) {
                        *PagesStartBmp = (UINT64)-1;
                        PagesStartBmp++;
                        NumPages -= 64;
                    }
                    if(NumPages) {
                        // Clear bits from 0 to Index
                        *(PagesStartBmp) |= ((UINT64)-1 << (Index + 1));
                        // SystemDebugPrint(L"C(%x, %x)", ((UINT64)-1 << (Index + 1)), *PagesStartBmp);
                    }
                }
                // SystemDebugPrint(L"PI : %d, I : %d, PBMP : %x", PagesStartIndex, Index, *PagesStartBmp);
                // for(UINT c = PagesStartIndex;c<=Index;c++) {
                //     _bittestandreset64(PagesStartBmp, c);
                // }
                _bittestandreset((long*)&AllocateContiguousMutex, 0);
                return PhysicalStart;
            }
            NextIndex = Index + 1;
            
        }
    }
    _bittestandreset((long*)&AllocateContiguousMutex, 0);
    return NULL;
}

// Return value : Start of those pages
void* ProcessAllocateVirtualMemory(RFPROCESS Process, UINT64 NumPages) {
    // register UINT64* Bitmap = (UINT64*)MemoryManagementTable.PageBitmap;
    // register UINT64 Size = MemoryManagementTable.NumBytesPageBitmap >> 3;
    // register INT64 VIndex = 0, PIndex = 0;
    // PAGE* PageArray = MemoryManagementTable.PageArray;
    // UINT MapFlags = PM_MAP;
    // if(Process->OperatingMode == USERMODE_PROCESS) MapFlags |= PM_USER;
    // for(register UINT64 i = 0;i<Size;i++, Bitmap++, PageArray += 64) {
    //     while((PIndex = __SyncBitmapAllocate(Bitmap)) != -1) {
    //         if(!NumPages) return NULL;
    //         PAGE* Page = PageArray + PIndex;
    //         PAGE_ALLOCATION_TABLE* Pat = &Process->MemoryManagementTable.PageAllocationTable;
    //         for(UINT64 l = 0;;l++) {
    //             UINT64 Vaddr = Process->MemoryManagementTable.VirtualMemoryBase + (l << 6);
                
    //             if((VIndex = __SyncBitmapAllocate(&Pat->Bitmap)) != -1) {
    //                 Pat->VirtualMemoryPages[VIndex].VirtualMemoryDescriptor = (UINT64)Page | 1;
    //                 MapPhysicalPages(Process->PageMap, (LPVOID)(Vaddr + (VIndex << 12)), (LPVOID)(Page->PageStruct.PhysicalPageNumber << 12), 1, MapFlags);
    //                 break;
    //             }
    //             if(!Pat->Next) {
    //                 Pat->Next = _SIMD_AllocatePhysicalPage(MemoryManagementTable.PageBitmap, MemoryManagementTable.NumBytesPageBitmap, MemoryManagementTable.PageArray);
    //                 if(!Pat->Next) SET_SOD_MEMORY_MANAGEMENT;
    //                 Pat->Next->Next = NULL;
    //                 Pat->Next->Bitmap = -1;
    //             }
    //             Pat = Pat->Next;
    //         }
    //         NumPages--;
    //     }
    // }
    return NULL;
}

BOOL KeAllocateFragmentedPages(RFPROCESS Process, void* VirtualMemory, UINT64 NumPages) {
    if(!Process || !NumPages) return FALSE;
    char* Off = VirtualMemory;
    PAGE* Page = MemoryManagementTable.PageArray;
    UINT64* Bitmap = (UINT64*)MemoryManagementTable.PageBitmap;
    register const UINT64 Max = MemoryManagementTable.NumBytesPageBitmap >> 3;
    unsigned long Index;
    for(UINT64 i = 0;i<Max;i++, Page+=64, Bitmap++) {
        UINT64 b = *Bitmap;
        if(b == (UINT64)-1) continue;
        if(b == 0) {
            *Bitmap = -1 << (64 - min(64, NumPages));
            for(UINT c = 0;c<min(64, NumPages);c++) {
                Page[c].PageStruct.Allocated = 1;
                MapPhysicalPages(Process->PageMap, Off, (LPVOID)(Page[c].PhysicalAddress & ~0xFFF), 1, PM_MAP);
                Off += 0x1000;
            }
            NumPages -= min(64, NumPages);
            if(!NumPages) goto Success;
            continue;
        }
        // // Scan for the first bit
        _BitScanForward64(&Index, b);
        if(Index) {
            b |= ((UINT64)-1 >> (64 - Index));
            for(UINT c = 0;c<min(Index, NumPages);c++) {
                Page[c].PageStruct.Allocated = 1;
                MapPhysicalPages(Process->PageMap, Off, (LPVOID)(Page[c].PhysicalAddress & ~0xFFF), 1, PM_MAP);
                Off += 0x1000;
            }
            NumPages -= min(Index, NumPages);
            if(!NumPages) goto Success;
        }
        // // Scan for the last bit
        _BitScanReverse64(&Index, b);
        if(Index != 63) {
            b |= ((UINT64)-1 << (Index + 1));
            for(UINT c = 0;c<min(64 - (Index + 1), NumPages);c++) {
                Page[c].PageStruct.Allocated = 1;
                MapPhysicalPages(Process->PageMap, Off, (LPVOID)(Page[c].PhysicalAddress & ~0xFFF), 1, PM_MAP);
                Off += 0x1000;
            }
            NumPages -= min(64 - Index - 1, NumPages);
            if(!NumPages) goto Success;
        }
        // Fill remaining bits
        while(_BitScanForward64(&Index, ~b)) {
            _bittestandset64((__int64*)&b, Index);
            NumPages--;
            Page[Index].PageStruct.Allocated = 1;
            MapPhysicalPages(Process->PageMap, Off, (LPVOID)(Page[Index].PhysicalAddress & ~0xFFF), 1, PM_MAP);
            Off += 0x1000;
            if(!NumPages) {
                *Bitmap = b;
                goto Success;
            }
        }
        *Bitmap = b;
    }
    // The allocation did not finish meaning that there is a bug in the system
    SET_SOD_MEMORY_MANAGEMENT;
    // ---------------
    Success:
        MemoryManagementTable.AllocatedPages+=NumPages;
        return TRUE;
}