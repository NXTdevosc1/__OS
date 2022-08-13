#include <ddk.h>
#include <ahci.h>

DDKSTATUS AhciSataRead(RFAHCI_DEVICE_PORT Port, UINT64 Address, UINT64 NumBytes, void* _Buffer) {
    char* Buffer = _Buffer;
    UINT64 Remaining = NumBytes;
    RFTHREAD Thread = KeGetCurrentThread();
    while(Remaining) {
        RFTHREAD* Th = Port->PendingCommands;
        for(UINT i = 0;i<Port->Ahci->MaxCommandSlots;i++, Th++) {
            // Clear all acked reads
            if(*Th == Thread && (Port->DoneCommands & (1 << i))) {
                // Release Command Slot
                Port->DoneCommands &= ~(1 << i);
                *Th = NULL;
            }
        }
        UINT64 ReadBytes = Remaining;
        if(Remaining > MAX_PRDT_BYTE_COUNT) {
            ReadBytes = MAX_PRDT_BYTE_COUNT;
            Remaining -= MAX_PRDT_BYTE_COUNT;
        } else {
            Remaining = 0;
        }

        UINT32 CommandSlot = AhciAllocateCommand(Port);
        // SystemDebugPrint(L"READ_ATTEMPT (CMD_SLOT: %d)", CommandSlot);
        AHCI_COMMAND_LIST_ENTRY* CmdEntry = &Port->CommandList[CommandSlot];
        AHCI_COMMAND_TABLE* CmdTbl = &Port->CommandTables[CommandSlot];
        CmdEntry->CommandFisLength = sizeof(ATA_FIS_H2D) >> 2;
        CmdEntry->PrdtByteCount = ReadBytes >> 9;
        CmdEntry->PrdtLength = 1;
        ATA_FIS_H2D* H2d = (ATA_FIS_H2D*)CmdTbl->CommandFis;
        H2d->FisType = FIS_TYPE_H2D;
        H2d->CommandControl = 1;
        H2d->Command = ATA_READ_DMA;
        H2d->Count = ReadBytes >> 9;
        H2d->Device = AHCI_DEVICE_LBA;
        H2d->Lba0 = Address;
        H2d->Lba1 = Address >> 16;
        H2d->Lba2 = Address >> 24;
        H2d->Lba3 = Address >> 40;

        CmdTbl->Prdt[0].DataBaseAddress = (UINT64)Buffer;
        CmdTbl->Prdt[0].DataByteCount = ReadBytes - 1;

        // Port->PendingCommandAccess = 1;
        Port->PendingCommands[CommandSlot] = Thread;
        // while(Port->Ahci->Interrupt);
        Port->Port->CommandIssue |= (1 << CommandSlot);
        // Port->PendingCommandAccess = 0;

        Buffer += ReadBytes;
    }
    for(;;) {
        
        // Waiting until read finishes
        RFTHREAD* Th = Port->PendingCommands;
        BOOL ThreadPending = FALSE;
        for(UINT i = 0;i<Port->Ahci->MaxCommandSlots;i++, Th++) {
            // Clear all acked reads
            if(*Th != Thread) continue;
            INTERRUPT_INFORMATION If = {0};
            If.Device = Port->Device;
            ThreadPending = TRUE;
            if((Port->DoneCommands & (1 << i))) {
                // Release Command Slot
                Port->DoneCommands &= ~(1 << i);
                *Th = NULL;
            }
        }
        if(!ThreadPending) break;
    }
    return KERNEL_SOK;
}

DDKSTATUS AhciSatapiRead(RFDEVICE_OBJECT Device, UINT64 Address, UINT64 NumBytes, void* Buffer) {
    return KERNEL_SERR;
}