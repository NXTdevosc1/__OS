#include <CPU/paging.h>
#include <MemoryManagement.h>
#include <cstr.h>
#include <interrupt_manager/SOD.h>
#include <kernel.h>
#include <CPU/cpu.h>
#include <preos_renderer.h>
#include <CPU/process.h>
#define ResetPageMap(x) memset((LPVOID)(x), 0, 0x1000)

void* GetPhysAddr(RFPROCESS Process, const void* Ptr){
    // from image
   
    UINT64 PageIndex = (UINT64)Ptr >> 12;
    UINT64 pt = PageIndex & 0x1FF;
    UINT64 pd = (PageIndex >> 9) & 0x1FF;
    UINT64 pdp = (PageIndex >> 18) & 0x1FF;
    UINT64 pml4 = (PageIndex >> 27) & 0x1FF;

    UINT64 cr3 = (UINT64)Process->PageMap;
    cr3 &= ~(0xFFF);
    RFPAGEMAP _pml4 = (RFPAGEMAP)(cr3 + (pml4 << 3));
    RFPAGEMAP _pdp = (RFPAGEMAP)((_pml4->PhysicalAddr << 12) + (pdp << 3));
    RFPAGEMAP _pd = (RFPAGEMAP)((_pdp->PhysicalAddr << 12) + (pd << 3));
    RFPAGEMAP _pt = (RFPAGEMAP)((_pd->PhysicalAddr << 12) + (pt << 3));
    return (void*)((_pt->PhysicalAddr << 12) + ((UINT64)Ptr & 0xFFF));
}



int MapPhysicalPages(
    RFPAGEMAP PageMap,
    LPVOID VirtualAddress,
    LPVOID PhysicalAddress,
    UINT64 Count,
    UINT64 Flags
){

    RFPAGEMAP Pml4Entry = (RFPAGEMAP)((UINT64)PageMap & ~(0xFFF)), PdpEntry = NULL, PdEntry = NULL, PtEntry = NULL;
    UINT64 TmpVirtualAddr = (UINT64)VirtualAddress >> 12;
    UINT64 TmpPhysicalAddr = (UINT64)PhysicalAddress >> 12;

    UINT64 Pml4Index = 0, PdpIndex = 0, PdIndex = 0, PtIndex = 0;
    UINT64 EntryAddr = 0;
    PTBLENTRY ModelEntry = { 0 };

    if (Flags & PM_PRESENT) ModelEntry.Present = TRUE;
    if (Flags & PM_READWRITE) ModelEntry.ReadWrite = TRUE;
    if (Flags & PM_USER) ModelEntry.UserSupervisor = TRUE;
    if (Flags & PM_NX) {
        // Check Execute Disable Compatiblity (If Reserved bit is set (NX on non compatible cpu)
        // A page fault will occur
        int edx = 0;
        CPUID_INFO CpuInfo = {0};
        __cpuidex(&CpuInfo, 0x80000001, 0);
        
        if (CpuInfo.edx & (1 << 20)) {
            ModelEntry.ExecuteDisable = TRUE; // CPU is NX (Data Execution Prevention) Compatible
        }
    }
    if (Flags & PM_GLOBAL) ModelEntry.Global = TRUE;
    if (Flags & PM_WRITE_THROUGH) ModelEntry.PageLevelWriteThrough = TRUE;
    if (Flags & PM_CACHE_DISABLE) ModelEntry.PageLevelCacheDisable = TRUE;


    for(UINT64 i = 0;i<Count;i++, TmpPhysicalAddr++, TmpVirtualAddr++){

        PtIndex = TmpVirtualAddr & 0x1FF;
        PdIndex = (TmpVirtualAddr >> 9) & 0x1FF;
        PdpIndex = (TmpVirtualAddr >> 18) & 0x1FF;
        Pml4Index = (TmpVirtualAddr >> 27) & 0x1FF;


        if(!Pml4Entry[Pml4Index].Present){
            EntryAddr = (UINT64)kpalloc(1);
            if(!EntryAddr) SET_SOD_MEMORY_MANAGEMENT;
            Pml4Entry[Pml4Index].PhysicalAddr = EntryAddr >> 12;
            Pml4Entry[Pml4Index].Present = 1;
            Pml4Entry[Pml4Index].ReadWrite = 1;
            Pml4Entry[Pml4Index].UserSupervisor = 1;
            

            ResetPageMap(EntryAddr);
        }else EntryAddr = (UINT64)Pml4Entry[Pml4Index].PhysicalAddr << 12;
        
        PdpEntry = (RFPAGEMAP)EntryAddr;

        if(!PdpEntry[PdpIndex].Present){
            EntryAddr = (UINT64)kpalloc(1);
            if(!EntryAddr) SET_SOD_MEMORY_MANAGEMENT;

            PdpEntry[PdpIndex].PhysicalAddr = EntryAddr >> 12;
            PdpEntry[PdpIndex].Present = 1;
            PdpEntry[PdpIndex].ReadWrite = 1;
            PdpEntry[PdpIndex].UserSupervisor = 1;

            ResetPageMap(EntryAddr);
        }else EntryAddr = (UINT64)PdpEntry[PdpIndex].PhysicalAddr << 12;
        PdEntry = (RFPAGEMAP)EntryAddr;
        if(!PdEntry[PdIndex].Present){
            EntryAddr = (UINT64)kpalloc(1);
            if(!EntryAddr) SET_SOD_MEMORY_MANAGEMENT;
            PdEntry[PdIndex].PhysicalAddr = EntryAddr >> 12;
            PdEntry[PdIndex].Present = 1;
            PdEntry[PdIndex].ReadWrite = 1;
            PdEntry[PdIndex].UserSupervisor = 1;

            ResetPageMap(EntryAddr);

        }else EntryAddr = (UINT64)PdEntry[PdIndex].PhysicalAddr << 12;

        PtEntry = (RFPAGEMAP)EntryAddr;

        *&PtEntry[PtIndex] = ModelEntry;

        PtEntry[PtIndex].PhysicalAddr = TmpPhysicalAddr;

        // Check if paging modification is not very big
        if(Count <= 0x2000 || !(Flags & PM_NO_TLB_INVALIDATION)){
            __InvalidatePage((void*)TmpVirtualAddr);
        }
    }
    // if there is big changes then reload CR3
    if(Count > 0x2000 && !(Flags & PM_NO_CR3_RELOAD)){
        __setCR3((UINT64)PageMap);
    }
    return 0;
}

RFPAGEMAP CreatePageMap(){
    RFPAGEMAP PageMap = kpalloc(1);
    if(!PageMap) SET_SOD_MEMORY_MANAGEMENT;
    ResetPageMap(PageMap);
    return PageMap;
}
