#include <CPU/ioapic.h>
#include <acpi/madt.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <kernel.h>

#define MAX_IOAPICS 0x100

ACPI_IOAPIC* TMP_IoApicList[MAX_IOAPICS] = {0};
ACPI_IOAPIC* SortedIoApics[MAX_IOAPICS] = {0};

UINT NumIoApics = 0;
void IoApicInit(ACPI_IOAPIC* IoApic){
    if(TMP_IoApicList[IoApic->IoApicId]) return;
    
    TMP_IoApicList[IoApic->IoApicId] = IoApic;
    NumIoApics++;
}

UINT GetNumIoApics(){
    return NumIoApics;
}

ACPI_IOAPIC* QueryIoApicByPhysicalId(unsigned char IoApicId){
    if(IoApicId >= MAX_IOAPICS) return NULL;
    if(TMP_IoApicList[IoApicId]){
        return TMP_IoApicList[IoApicId];
    }
    return NULL;
}

void SortIoApics(){
    UINT NumSortedIoApics = 0;
    IOAPIC_REDTBL RedirectionEntry = {0};
    RedirectionEntry.InterruptMask = 1;
    for(UINT i = 0;i<MAX_IOAPICS;i++){
        if(NumSortedIoApics == NumIoApics) break;
        ACPI_IOAPIC* IoApic = NULL;
        IoApic = QueryIoApicByPhysicalId((UINT8)i);
        if(IoApic){
            MapPhysicalPages(kproc->PageMap, DWVOID IoApic->IoApicAddress, DWVOID IoApic->IoApicAddress, 1, PM_MAP | PM_CACHE_DISABLE);
            SortedIoApics[NumSortedIoApics] = IoApic;
            NumSortedIoApics++;
            // now mask all interrupt sources
            UINT MaxRedirectionEntry = IoApicGetMaxRedirectionEntry(IoApic);
            SystemDebugPrint(L"IOAPIC #%d MAX_REDIR_ENTRY : %d, GSYS_INTBASE : %d", IoApic->IoApicId, MaxRedirectionEntry, IoApic->GlobalSysInterruptBase);
            for(UINT c = 0;c<MaxRedirectionEntry + 1;c++){
                IoApicWriteRedirectionTable(IoApic, (UINT8)c, &RedirectionEntry);
            }
            
        }
    }
    if(NumSortedIoApics != NumIoApics) SOD(SOD_INTERRUPT_MANAGEMENT, "KERNEL MEMORY BUG OR CORRUPTED, NUM_SORTED_IOAPICs != NUM_IOAPICs");

}

ACPI_IOAPIC* QueryIoApic(unsigned char VirtualIoApicId){
    if(VirtualIoApicId >= NumIoApics) return NULL;
    return SortedIoApics[VirtualIoApicId];
}

UINT32 IoApicRead(ACPI_IOAPIC* IoApic, unsigned char AddressOffset){
    *(UINT32*)((UINT64)IoApic->IoApicAddress + IOAPIC_REGSEL) = (UINT32)AddressOffset;
    return *(UINT32*)((UINT64)IoApic->IoApicAddress + IOAPIC_WIN);
}

void IoApicWrite(ACPI_IOAPIC* IoApic, unsigned char AddressOffset, UINT32 Value){
    *(UINT32*)((UINT64)IoApic->IoApicAddress + IOAPIC_REGSEL) = (UINT32)AddressOffset;
    *(UINT32*)((UINT64)IoApic->IoApicAddress + IOAPIC_WIN) = Value;
}

void IoApicReadRedirectionTable(ACPI_IOAPIC* IoApic, unsigned char RedirectionTableIndex, RFIOAPIC_REDTBL RedirectionTable)
{
    if(RedirectionTableIndex > IoApicGetMaxRedirectionEntry(IoApic)) SOD(SOD_INTERRUPT_MANAGEMENT, "IOAPIC REDIR ENTRY > MAX_REDIR_ENTRIES (MAY BE A BUG, CONTACT US)");
    QWORD* RFQRedTbl = (QWORD*)RedirectionTable;
    QWORD  QRedTbl = IoApicRead(IoApic, IOAPIC_REDIRECTION_TABLE_BASE + (RedirectionTableIndex << 1));
    DWORD High = IoApicRead(IoApic, IOAPIC_REDIRECTION_TABLE_BASE + (RedirectionTableIndex << 1) + 1);
    QRedTbl |= ((QWORD)High << 32);
    *RFQRedTbl = QRedTbl;
}
void IoApicWriteRedirectionTable(ACPI_IOAPIC* IoApic, unsigned char RedirectionTableIndex, RFIOAPIC_REDTBL RedirectionTable){
    if(RedirectionTableIndex > IoApicGetMaxRedirectionEntry(IoApic)) SOD(SOD_INTERRUPT_MANAGEMENT, "IOAPIC REDIR ENTRY > MAX_REDIR_ENTRIES (MAY BE A BUG, CONTACT US)");
    QWORD QRedTbl = *(QWORD*)RedirectionTable;
    IoApicWrite(IoApic, IOAPIC_REDIRECTION_TABLE_BASE + (RedirectionTableIndex << 1), (UINT32)QRedTbl);
    IoApicWrite(IoApic, IOAPIC_REDIRECTION_TABLE_BASE + (RedirectionTableIndex << 1) + 1, (UINT32)(QRedTbl >> 32));
}

unsigned char IoApicGetMaxRedirectionEntry(ACPI_IOAPIC* IoApic){
    UINT IoApicVer = IoApicRead(IoApic, IOAPIC_VER);
    return ((IoApicVer >> 16) & 0xff);
}