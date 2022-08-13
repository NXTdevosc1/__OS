#include <ddk.h>
#include <ahci.h>

int AhciIssueCommand(RFAHCI_DEVICE_PORT Port, UINT CommandIndex) {
    if(Port->PendingCommands[CommandIndex]) return -1; // Command slot is already in use
    
    Port->PendingCommands[CommandIndex] = KeGetCurrentThread();


    SystemDebugPrint(L"AHCI : COMMAND_READY (PxIS : %x PxSERR : %x PxSCTL : %x PxSSTS : %x)", Port->Port->InterruptStatus, Port->Port->SataError, Port->Port->SataControl, Port->Port->SataStatus);
    __SyncOr(&Port->Port->CommandIssue, CommandIndex);
    while(!(Port->DoneCommands & (1 << CommandIndex))) IoWait();
    
    // Release Command Slot
    Port->DoneCommands &= ~(1 << CommandIndex);
    Port->PendingCommands[CommandIndex] = NULL;
    SystemDebugPrint(L"AHCI_COMMAND_ACK_RECEIVED. (PxIS : %x PxSERR : %x PxSCTL : %x PxSSTS : %x)", Port->Port->InterruptStatus, Port->Port->SataError, Port->Port->SataControl, Port->Port->SataStatus);
    return 0;
}

UINT32 AhciAllocateCommand(RFAHCI_DEVICE_PORT Port){
    
    for(;;) {
    DWORD Mask = Port->Port->CommandIssue | Port->Port->SataActive;
        // Spin until a free command is found
        for(UINT i = 0;i<Port->Ahci->MaxCommandSlots;i++){
            if(!(Mask & 1) && !(Port->UsedCommandSlots & (1 << i))) {
                // It is a free command list
                if(!__SyncBitTestAndSet(&Port->UsedCommandSlots, i)) continue;
                return i;
            }
            Mask >>= 1;
        }
    }
}
