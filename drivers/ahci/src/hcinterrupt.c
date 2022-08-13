#include <ddk.h>
#include <ahci.h>


void AhciInterruptHandler(RFDRIVER_OBJECT Driver, RFINTERRUPT_INFORMATION InterruptInformation) {

    RFAHCI_DEVICE Ahci = GetDeviceExtension(InterruptInformation->Device);
    UINT32 GlobalInterruptStatus = Ahci->Hba->InterruptStatus;
    // SystemDebugPrint(L"AHCI_INT : GS (%x)", GlobalInterruptStatus);

    UINT32 HbaReceivedIntStatus = 0;
   
    if(!GlobalInterruptStatus) return;

    Ahci->Hba->InterruptStatus = GlobalInterruptStatus;
    
    // Ahci->Interrupt = 1;
    for(int i = 0;i<Ahci->NumPorts;i++) {
        if(/*!Ahci->Ports[i].PendingCommandAccess && */(GlobalInterruptStatus & 1)) {
            HbaReceivedIntStatus |= (1 << i);
            HBA_PORT* HbaPort = &Ahci->HbaPorts[i];
            AHCI_DEVICE_PORT* Port = &Ahci->Ports[i];
            // SystemDebugPrint(L"Interrupt on PORT %d (%x)", (UINT64)i, HbaPort->PortSignature);
            if(HbaPort->InterruptStatus.D2HRegisterFisInterrupt || HbaPort->InterruptStatus.PioSetupFisInterrupt) {
                
                    
                // SystemDebugPrint(L"D2H_REG_FIS_INT (TYPE : %x) (CI : %x) (SAC : %x)", Ahci->Ports[i].ReceivedFis->D2h.FisType, HbaPort->CommandIssue, HbaPort->SataActive);
                if(!Port->FirstD2h) Port->FirstD2h = 1;
                DWORD CmdIssue = HbaPort->CommandIssue;
                DWORD iCmdIssue = CmdIssue; // Initial (Recevied) CI
                RFTHREAD* Pending = Ahci->Ports[i].PendingCommands;

                for(UINT8 c = 0;c<Ahci->MaxCommandSlots;c++, Pending++) {
                    if(*Pending && !(CmdIssue & 1)) {
                        // SystemDebugPrint(L"AHCI : Command#%d Compeleted", c);
                        RFTHREAD Th = *Pending;
                        Ahci->Ports[i].DoneCommands |= (1 << c);
                        Port->UsedCommandSlots &= ~(1 << c);
                        IoFinish(Th); // in case that the command is synchronized
                    }
                    CmdIssue >>= 1;
                }
                // OTHERWISE WAIT FOR NEXT D2H
            }
            if(HbaPort->InterruptStatus.D2HRegisterFisInterrupt) {
                HbaPort->InterruptStatus.D2HRegisterFisInterrupt = 1; // ACK INTERRUPT
            }

            // Usually by IDENTIFY_DEVICE_DATA Command
            if(HbaPort->InterruptStatus.PioSetupFisInterrupt) {
                SystemDebugPrint(L"PIO_SETUP");
                // memcpy(Port->ReceivedFis.)
                HbaPort->InterruptStatus.PioSetupFisInterrupt = 1;
            }
            if(HbaPort->InterruptStatus.DmaSetupFisInterrupt) {
                SystemDebugPrint(L"DMA_SETUP_FIS");
                HbaPort->InterruptStatus.DmaSetupFisInterrupt = 1;
            }


            if(HbaPort->InterruptStatus.SetDeviceBitsInterrupt) {
                SystemDebugPrint(L"SET_DEVICE_BITS");
                HbaPort->InterruptStatus.SetDeviceBitsInterrupt = 1;
            }
            if(HbaPort->InterruptStatus.UnknownFisInterrupt) {
                SystemDebugPrint(L"UNKNOWN_FIS");
                HbaPort->SataError.UnknownFisType = 0;
            }
            if(HbaPort->InterruptStatus.DescriptorProcessed) {
                SystemDebugPrint(L"DESCRIPTOR_PROCESSED");
                HbaPort->InterruptStatus.DescriptorProcessed = 1;
            }
            if(HbaPort->InterruptStatus.PortConnectChangeStatus) {
                SystemDebugPrint(L"PORT_CONNECT_CHANGE");
                Port->Port->SataError.Exchanged = 1;
                Port->Port->SataControl.DeviceDetectionInitialization = 0;
                
            }
            if(HbaPort->InterruptStatus.DeviceMechanicalPresenceStatus) SystemDebugPrint(L"DEVICE_MECHANICAL_PRESENCE");
            if(HbaPort->InterruptStatus.PhyRdyChangeStatus) {
                SystemDebugPrint(L"PHY_RDI_CHANGE");
                Port->Port->SataError.PhyRdyChange = 1;
                Port->FirstD2h = 1; // Some hardware does not send first D2H (instead it is detected with TaskFileData)
            } 
            if(HbaPort->InterruptStatus.IncorrectPortMultiplierStatus) SystemDebugPrint(L"INCORRECT_PORT_MULTIPLIER");
            if(HbaPort->InterruptStatus.OverflowStatus) SystemDebugPrint(L"OVERFLOW");
            if(HbaPort->InterruptStatus.InterfaceNonFatalErrorStatus) {
                SystemDebugPrint(L"INTERFACE_NON_FATAL");
                HbaPort->InterruptStatus.InterfaceNonFatalErrorStatus = 1;
            }
            if(HbaPort->InterruptStatus.InterfaceFatalErrorStatus) {
                SystemDebugPrint(L"INTERFACE_FATAL");
                HbaPort->InterruptStatus.InterfaceFatalErrorStatus = 1;
            }
            if(HbaPort->InterruptStatus.HostBusDataErrorStatus) SystemDebugPrint(L"HOST_BUS_DATA_ERROR");
            if(HbaPort->InterruptStatus.HostBusFatalErrorStatus) SystemDebugPrint(L"HOST_BUS_FATAL_ERROR");
            if(HbaPort->InterruptStatus.TaskFileErrorStatus) {
                SystemDebugPrint(L"TASK_FILE_ERROR");
                HbaPort->InterruptStatus.TaskFileErrorStatus = 1;
            }
            if(HbaPort->InterruptStatus.ColdPortDetectStatus) SystemDebugPrint(L"COLD_PRESENCE_DETECT");
        
        

        }
        GlobalInterruptStatus >>= 1;
    }
    // SystemDebugPrint(L"INTH_COMPLETE (Native Command Queuing : %d)", (UINT64)Ahci->Hba->HostCapabilities.SupportsNativeCommandQueuing);
    // Ahci->Hba->InterruptStatus = HbaReceivedIntStatus;
    // Ahci->Interrupt = 0;
}