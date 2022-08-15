#include <interrupt_manager/idt.h>
#include <MemoryManagement.h>
#include <interrupt_manager/interrupts.h>
#include <CPU/process.h>
#include <preos_renderer.h>
#include <cstr.h>
#include <Management/runtimesymbols.h>
#include <CPU/cpu.h>
#include <interrupt_manager/SOD.h>
#include <CPU/ioapic.h>
#include <IO/pcidef.h>
#include <IO/pci.h>

struct IDTR idtr = { 0 };
__declspec(align(0x1000)) struct INTERRUPT_DESCRIPTOR_TABLE KERNEL_IDT = { 0 };
static inline void IDT_set_offset(struct IDT_ENTRY* entry, uint64_t handler){
	entry->offset_low = handler;
	entry->offset_mid = handler >> 16;
	entry->offset_high = handler >> 32;
}

void SetInterruptGate(
	void* handler, uint8_t offset, uint8_t dpl, uint8_t ist, uint8_t type, uint16_t cs
){
	if(!handler) SET_SOD_INTERRUPT_MANAGEMENT;
	KERNEL_IDT.entries[offset].present = 1;
	IDT_set_offset(&KERNEL_IDT.entries[offset], (uint64_t)handler);
	KERNEL_IDT.entries[offset].dpl = dpl;
	KERNEL_IDT.entries[offset].ist = ist;
	KERNEL_IDT.entries[offset].segment_selector = cs;
	KERNEL_IDT.entries[offset].type = type;
}

void KERNELAPI RegisterInterruptServiceRoutine(INTERRUPT_SERVICE_ROUTINE Handler, UINT8 Offset) {
	SetInterruptGate(GlobalWrapperPointer[Offset], Offset, 0, 1, IDT_INTERRUPT_GATE, 0x08);
	if (!Handler) SET_SOD_INTERRUPT_MANAGEMENT;
	GlobalIsrPointer[Offset] = Handler;
}

#define MSI_TEST_IRQ 0x60

extern void SchedulerEntrySSE();
extern void SchedulerEntryAVX();
extern void SchedulerEntryAVX512();

void GlobalInterruptDescriptorInitialize()
{

	idtr.limit = 0xFFF; // limit - 1
	idtr.offset = (uint64_t)&KERNEL_IDT;	

	for (unsigned char i = 0; i < 0x20; i++) {
		SetInterruptGate(GlobalWrapperPointer[i], i, 0, 1, IDT_INTERRUPT_GATE, 0x08);
	}

	for(unsigned char i = 0;i<0x20;i++){
		SetInterruptGate(gIRQCtrlWrapPtr[i], 0x20 + i, 0, 3, IDT_INTERRUPT_GATE, 0x08);
	}
	
	

	SetInterruptGate(GlobalWrapperPointer[CPU_INTERRUPT_BREAK_POINT], CPU_INTERRUPT_BREAK_POINT, 3, 1, IDT_INTERRUPT_GATE, 0x08);
	

	// System & CPU Interrupts
	RegisterInterruptServiceRoutine(INTH_DIVIDED_BY_0, CPU_INTERRUPT_DIVIDED_BY_0);

	RegisterInterruptServiceRoutine(INTH_DEBUG_EXCEPTION, CPU_INTERRUPT_DEBUG_EXCEPTION);

	RegisterInterruptServiceRoutine(INTH_NON_MASKABLE_INTERRUPT,		CPU_INTERRUPT_NON_MASKABLE_INTERRUPT);
	RegisterInterruptServiceRoutine(INTH_INTO_DETECTED_OVERFLOW,		CPU_INTERRUPT_INTO_DETECTED_OVERFLOW);
	RegisterInterruptServiceRoutine(INTH_OUT_OF_BOUNDS_EXCEPTION,		CPU_INTERRUPT_OUT_OF_BOUNDS_EXCEPTION);
	RegisterInterruptServiceRoutine(INTH_INVALID_OPCODE_EXCEPTION,		CPU_INTERRUPT_INVALID_OPCODE_EXCEPTION);
	RegisterInterruptServiceRoutine(INTH_NO_COPROCESSOR_EXCEPTION,		CPU_INTERRUPT_NO_COPROCESSOR_EXCEPTION);
	RegisterInterruptServiceRoutine(INTH_INVALID_TSS,					CPU_INTERRUPT_INVALID_TSS);
	RegisterInterruptServiceRoutine(INTH_SEGMENT_NOT_PRESENT,			CPU_INTERRUPT_SEGMENT_NOT_PRESENT);
	RegisterInterruptServiceRoutine(INTH_STACK_FAULT,					CPU_INTERRUPT_STACK_FAULT);
	RegisterInterruptServiceRoutine(INTH_GENERAL_PROTECTION_FAULT,		CPU_INTERRUPT_GENERAL_PROTECTION_FAULT);
	RegisterInterruptServiceRoutine(INTH_PAGE_FAULT,					CPU_INTERRUPT_PAGE_FAULT);
	RegisterInterruptServiceRoutine(INTH_COPROCESSOR_FAULT,			CPU_INTERRUPT_COPROCESSOR_FAULT);
	RegisterInterruptServiceRoutine(INTH_ALIGNMENT_CHECK_EXCEPTION,	CPU_INTERRUPT_ALIGNMENT_CHECK_EXCEPTION);
	RegisterInterruptServiceRoutine(INTH_MACHINE_CHECK_EXCEPTION,		CPU_INTERRUPT_MACHINE_CHECK_EXCEPTION);
	RegisterInterruptServiceRoutine(INTH_DBL_FAULT,					CPU_INTERRUPT_DOUBLE_FAULT);
	RegisterInterruptServiceRoutine(INTH_SIMD_FLOATING_POINT_EXCEPTION, CPU_INTERRUPT_SIMD_FLOATING_POINT_EXCEPTION);
	
	RegisterInterruptServiceRoutine(INTH_UNKNOWN_INTERRUPT_EXCEPTION, CPU_INTERRUPT_UNKOWN_INTERRUPT_EXCEPTION);

	RegisterInterruptServiceRoutine(INTH_IPI, IPI_INTVEC);
	
	// User Permitted Interrupts

	RegisterInterruptServiceRoutine(INTH_BREAK_POINT, CPU_INTERRUPT_BREAK_POINT);

	
	
	// IRQ

	// Determine which function to use depending on extension support :
	// (SSE, AVX, AVX512)
	CPUID_INFO CpuInfo = {0};
	__cpuid(&CpuInfo, 1);
	// if(CpuInfo.ecx & CPUID1_ECX_AVX) {
	// 	SOD(0, "Process Supports AVX");
	// 	while(1);
	// 	SetInterruptGate(SchedulerEntryAVX,INT_APIC_TIMER,0, 2, IDT_INTERRUPT_GATE, CS_KERNEL);
	// } else {
		SetInterruptGate(SchedulerEntrySSE,INT_APIC_TIMER,0, 2, IDT_INTERRUPT_GATE, CS_KERNEL);
	// }
	
	// SetInterruptGate(SkipTaskSchedule,INT_SCHEDULE,0, 2, IDT_INTERRUPT_GATE, CS_KERNEL);

	SetInterruptGate(ApicSpuriousInt, INT_APIC_SPURIOUS, 0, 3, IDT_INTERRUPT_GATE, 0x08);
	RegisterInterruptServiceRoutine(ApicThermalSensorInt, INT_TSR);
	RegisterInterruptServiceRoutine(ApicPerformanceMonitorCountersInt, INT_PMCR);
	RegisterInterruptServiceRoutine(ApicLint0Int, INT_LINT0);
	RegisterInterruptServiceRoutine(ApicLint1Int, INT_LINT1);
	RegisterInterruptServiceRoutine(ApicLvtErrorInt, INT_LVT_ERROR);
	RegisterInterruptServiceRoutine(ApicCmciInt, INT_CMCI);


	
	//SetInterruptGate(__InterruptCheckHalt, CPU_INTERRUPT_GENERAL_PROTECTION_FAULT, 0, 1, IDT_INTERRUPT_GATE, 0x08);
	//SetInterruptGate(__InterruptCheckHalt, CPU_INTERRUPT_PAGE_FAULT, 0, 1, IDT_INTERRUPT_GATE, 0x08);

	CreateRuntimeSymbol("$_InterruptDescriptorTable", &KERNEL_IDT);
	CreateRuntimeSymbol("$_InterruptDescriptorRegister", &idtr);
	CreateRuntimeSymbol(".$_GlobalIsrPtr", GlobalIsrPointer);

	// Remap PIC
	remap_pic();
	// Mask All PIC Interrupts
	OutPortB(0xA1, 0xFF);
	OutPortB(0x21, 0xFF);

	
}

void GlobalInterruptDescriptorLoad() {
	__lidt(&idtr);
}




uint64_t IDT_get_offset(struct IDT_ENTRY* entry){
	uint64_t offset = 0;
	offset |= (uint64_t)entry->offset_low;
	offset |= (uint64_t)entry->offset_mid << 16;
	offset |= (uint64_t)entry->offset_high << 32;
	return offset;
}

void remap_pic(){
	
	// get PIC masks
	uint8_t a1 = InPortB(PIC1_DATA);
	uint8_t a2 = InPortB(PIC2_DATA);

	// Init ICW1
	OutPortB(PIC1_COMMAND,ICW1_INIT | ICW1_ICW4);
	OutPortB(PIC2_COMMAND,ICW1_INIT | ICW1_ICW4);
	
	// Init ICW2
	OutPortB(PIC1_DATA,0x20);
	OutPortB(PIC2_DATA,0x28);

	// Init ICW3
	OutPortB(PIC1_DATA, 4);
	OutPortB(PIC2_DATA,2);

	// Init ICW4
	// Give additionnal info about pic environnement
	OutPortB(PIC1_DATA,ICW4_8086);
	OutPortB(PIC2_DATA,ICW4_8086);

	// Mask Interrupts
	// Restore previously saved masks

	OutPortB(PIC1_DATA,a1);
	OutPortB(PIC2_DATA,a2);

}

UINT gIRQ_MGR_ProcessorSwitch = 0; // ProcessorId Switch

UINT64 gIRQ_MGR_MUTEX = 0;

KERNELSTATUS KERNELAPI KeControlIrq(KEIRQ_ROUTINE Handler, UINT IrqNumber, UINT DeliveryMode, UINT Flags){
	if(!Handler || DeliveryMode > 3) return KERNEL_SERR_INVALID_PARAMETER;
	__SpinLockSyncBitTestAndSet(&gIRQ_MGR_MUTEX, 0);
	UINT64 _rflags = __getRFLAGS();
	__cli();
	if(gIRQ_MGR_ProcessorSwitch == Pmgrt.NumProcessors) gIRQ_MGR_ProcessorSwitch = 0;
	UINT NumIoApics = GetNumIoApics();
	if(!(Flags & IRQ_CONTROL_DISABLE_OVERRIDE_CHECK)){
		UINT RedirectedIrqFlags = 0;
		UINT RedirectedIrq = GetRedirectedIrq(IrqNumber, &RedirectedIrqFlags);
		if(RedirectedIrq != 0){
			// SystemDebugPrint(L"Detected Redirected IRQ from %d to %d (Flags = %x)", IrqNumber, RedirectedIrq, RedirectedIrqFlags);
			if(!(Flags & IRQ_CONTROL_DISABLE_OVERRIDE_FLAGS)){
				// Set the new flags
				Flags &= ~0x1F;
				if(MULTIBIT_TEST(RedirectedIrqFlags, ISO_ACTIVE_LOW)) {
					// SystemDebugPrint(L"ACTIVE_LOW");
					Flags |= IRQ_CONTROL_LOW_ACTIVE;
				}
				if(!MULTIBIT_TEST(RedirectedIrqFlags, ISO_EDGE_TRIGGERED)) {
					// SystemDebugPrint(L"EDGE_TRIGGERED");
					Flags |= IRQ_CONTROL_LEVEL_SENSITIVE;
				}
			}
			IrqNumber = RedirectedIrq;
		}
	}
	if(IrqNumber > 24 * NumIoApics) {
		__BitRelease(&gIRQ_MGR_MUTEX, 0);
		// Restore RFLAGS
		__setRFLAGS(_rflags);
		return KERNEL_SERR_OUT_OF_RANGE;
	}
	// just as pre-check

	// Check if IRQ is controlled by another process
	void* Process = (void*)KeGetCurrentProcess();

	for(UINT x = 0;x<Pmgrt.NumProcessors;x++){
		for(UINT i = 0;i<NUM_IRQS_PER_PROCESSOR;i++){
			if(CpuManagementTable[x]->IrqControlTable[i].Present &&
			CpuManagementTable[x]->IrqControlTable[i].PhysicalIrqNumber == IrqNumber
			) {
				if(CpuManagementTable[x]->IrqControlTable[i].Process != Process) {
					__BitRelease(&gIRQ_MGR_MUTEX, 0);
					__setRFLAGS(_rflags);
					return KERNEL_SERR_IRQ_CONTROLLED_BY_ANOTHER_PROCESS;
				}
				CpuManagementTable[x]->IrqControlTable[i].Flags = Flags;
				__BitRelease(&gIRQ_MGR_MUTEX, 0);
				__setRFLAGS(_rflags);

				return KERNEL_SWR_IRQ_ALREADY_SET;
			}
		}
	}

	UINT IrqIndex = IrqNumber;
	BOOL _redirentryfound = FALSE;
	ACPI_IOAPIC* IoApic = NULL;
	UINT VirtualIoApicId = 0;


	for(UINT i = 0;i<NumIoApics;i++){
		// get aligned ids (for e.g. IOAPIC 0 May be 5, and IOAPIC1 May be 6)
		IoApic = QueryIoApic(i);
		unsigned char MaxRedirectionEntry = IoApicGetMaxRedirectionEntry(IoApic);
		// SystemDebugPrint(L"IOAPIC#%d Max Redirection Entry : %d, GSYS_INT_BASE : %d", IoApic->IoApicId, MaxRedirectionEntry);
		if(MaxRedirectionEntry >= IrqIndex) {
			_redirentryfound = TRUE;
			VirtualIoApicId = i;
			break;
		}
		IrqIndex -= (MaxRedirectionEntry + 1); // Num Redir entries
	}

	if(!_redirentryfound) {
		__BitRelease(&gIRQ_MGR_MUTEX, 0);
		__setRFLAGS(_rflags);
		return KERNEL_SERR_OUT_OF_RANGE;
	}

	CPU_MANAGEMENT_TABLE* CpuManagementTbl = CpuManagementTable[gIRQ_MGR_ProcessorSwitch];
	// Allocate IRQ Control Descriptor
	IRQ_CONTROL_DESCRIPTOR* IrqControlDescriptor = NULL;
	for(UINT i = 0;i<NUM_IRQS_PER_PROCESSOR;i++){
		if(!CpuManagementTbl->IrqControlTable[i].Present){
			IrqControlDescriptor = &CpuManagementTbl->IrqControlTable[i];
			IrqControlDescriptor->InterruptVector = 0x20 + i;
			break;
		}
	}
	if(!IrqControlDescriptor) SOD(SOD_INTERRUPT_MANAGEMENT, "FAILED TO ALLOCATE IRQ HANDLER");
	IrqControlDescriptor->VirtualIoApicId = VirtualIoApicId;
	IrqControlDescriptor->PhysicalIrqNumber = IrqNumber;
	IrqControlDescriptor->IoApicIrqNumber = IrqIndex;
	IrqControlDescriptor->Flags = Flags;
	IrqControlDescriptor->Process = KeGetCurrentProcess();
	IrqControlDescriptor->Handler = Handler;
	IrqControlDescriptor->Source = INTERRUPT_SOURCE_IRQ;
	IrqControlDescriptor->Present = TRUE;
	// Unmask IOAPIC Redirection Entry
	
	IOAPIC_REDTBL RedirectionTable = {0};
	RedirectionTable.InterruptVector = IrqControlDescriptor->InterruptVector;
	RedirectionTable.DestinationField = gIRQ_MGR_ProcessorSwitch;
	RedirectionTable.DestinationMode = 0;
	if(DeliveryMode == IRQ_DELIVERY_NMI){
		RedirectionTable.DeliveryMode = 4;
	}else if(DeliveryMode == IRQ_DELIVERY_SMI){
		RedirectionTable.DeliveryMode = 2;
	}else if(DeliveryMode == IRQ_DELIVERY_EXTINT){
		RedirectionTable.DeliveryMode = 7;
	}


	if(Flags & IRQ_CONTROL_LOW_ACTIVE){
		RedirectionTable.InterruptInputPinPolarity = 1;
	}else {
		RedirectionTable.InterruptInputPinPolarity = 0;
	}
	if(!(Flags & IRQ_CONTROL_LEVEL_SENSITIVE)){
		RedirectionTable.TriggerMode = 0;
	}else RedirectionTable.TriggerMode = 1;

	if(Flags & IRQ_CONTROL_USE_BASIC_INTERRUPT_WRAPPER) {
		// SystemDebugPrint(L"USE_BASIC_WRAPPER : IV (%d)", (UINT64)IrqControlDescriptor->InterruptVector);
		RegisterInterruptServiceRoutine((INTERRUPT_SERVICE_ROUTINE)Handler, IrqControlDescriptor->InterruptVector);
	} else {
		// SystemDebugPrint(L"IRQ_WRAPPER : IRQ %d | IV : %d", (UINT64)IrqControlDescriptor->PhysicalIrqNumber, (UINT64)IrqControlDescriptor->InterruptVector);
		SetInterruptGate(gIRQCtrlWrapPtr[IrqControlDescriptor->InterruptVector - 0x20], IrqControlDescriptor->InterruptVector, 0, 3, IDT_INTERRUPT_GATE, 0x08);
	}
	IoApicWriteRedirectionTable(IoApic, IrqControlDescriptor->IoApicIrqNumber, &RedirectionTable);
	
	// SystemDebugPrint(L"IOAPIC#%d Modified (INT_VEC : 0x%x DEST : %d HANDLER: 0x%x PHYS_IRQ: 0x%x)", IoApic->IoApicId, RedirectionTable.InterruptVector, RedirectionTable.DestinationField, IrqControlDescriptor->Handler, (UINT64)IrqControlDescriptor->PhysicalIrqNumber);

	gIRQ_MGR_ProcessorSwitch++;
	__BitRelease(&gIRQ_MGR_MUTEX, 0);
	__setRFLAGS(_rflags);
	return KERNEL_SOK;
}
KERNELSTATUS KERNELAPI KeReleaseIrq(UINT IrqNumber){
	return KERNEL_SOK;
}

IRQ_CONTROL_DESCRIPTOR* KERNELAPI RegisterIrq(UINT32 IrqSource, void* Handler, UINT32 Flags, UINT32 IoApicId) {
	__SpinLockSyncBitTestAndSet(&gIRQ_MGR_MUTEX, 0);
	UINT32 Dec = 2;
	for(;;) {
	for(UINT i = gIRQ_MGR_ProcessorSwitch;i<Pmgrt.NumProcessors;i++) {
		IRQ_CONTROL_DESCRIPTOR* CpuMt = CpuManagementTable[i]->IrqControlTable;
		for(UINT c = 0;c<NUM_IRQS_PER_PROCESSOR;c++, CpuMt++) {
			if(!CpuMt->Present) {
				CpuMt->Flags = Flags;
				CpuMt->InterruptVector = c + 0x20;
				SetInterruptGate(gIRQCtrlWrapPtr[c], CpuMt->InterruptVector, 0, 3, IDT_INTERRUPT_GATE, 0x08);
				CpuMt->PhysicalIrqNumber = IrqSource;
				CpuMt->Process = KeGetCurrentProcess();
				CpuMt->VirtualIoApicId = IoApicId;
				CpuMt->LapicId = CpuManagementTable[i]->ProcessorId;
				CpuMt->Handler = Handler;
				CpuMt->Present = 1;
				gIRQ_MGR_ProcessorSwitch = c + 1;
				__BitRelease(&gIRQ_MGR_MUTEX, 0);
				return CpuMt;
			}
		}
	}
	gIRQ_MGR_ProcessorSwitch = 0;
	Dec--;
	if(!Dec) break;
	}
	// TODO : If MSI (Create a global IRQ)
	__BitRelease(&gIRQ_MGR_MUTEX, 0);
	return NULL;
}

KERNELSTATUS KERNELAPI SetInterruptService(RFDEVICE_OBJECT Device, KEIRQ_ROUTINE Handler) {
	// if(!ValidateDevice(Device)) return KERNEL_SERR_INVALID_PARAMETER;
	
	// SystemDebugPrint(L"SET_INT_SERVICE (Device : %x) (Handler : %x)", Device, Handler);

	UINT16 PciStatus = PciDeviceConfigurationRead16(Device, PCI_STATUS);

	// Enable device interrupts
	UINT16 Command = PciDeviceConfigurationRead16(Device, PCI_COMMAND);
	// SystemDebugPrint(L"PCI Command (BEFORE) : %x, STATUS : %x", (UINT64)Command, (UINT64)PciStatus);
	Command &= ~0x400; // Remove Interrupt disable
	Command |= (7); // Set Bus master & IO-Space & Memory Space
	

	// SystemDebugPrint(L"(TARGET) PCI Command : %x", (UINT64)Command);

	PciDeviceConfigurationWrite16(Device, Command , PCI_COMMAND);
	Command = PciDeviceConfigurationRead16(Device, PCI_COMMAND);
	
	PciStatus = PciDeviceConfigurationRead16(Device, PCI_STATUS);

	
	// SystemDebugPrint(L"PCI Command : %x, STATUS : %x", (UINT64)Command, (UINT64)PciStatus);

	if(PciStatus & PCI_STATUS_CAPABILITES_LIST) {
            // Capabilites Supported
            UINT16 CapOffset = PciDeviceConfigurationRead16(Device, PCI_CAPABILITIES_PTR);
            UINT16 _caphdr = PciDeviceConfigurationRead16(Device, CapOffset);
            PCI_CAPABILITY_HEADER* CapabilityHeader = (PCI_CAPABILITY_HEADER*)&_caphdr;
            DWORD CapBuffer[0x40] = {0};
            for(;;) {
                if(CapabilityHeader->CapabilityId == PCI_CAPABILITY_ID_MSI) {
					SystemDebugPrint(L"MSI Capability Found");
                    for(UINT i = 0;i<sizeof(PCI_MSI_CAPABILITY_64BIT) >> 2;i++){
                        CapBuffer[i] = PciDeviceConfigurationRead32(Device, CapOffset + (i << 2));
                    }

                    PCI_MSI_CAPABILITY* Msi = (PCI_MSI_CAPABILITY*)CapBuffer;
                    
					// First disable msi
					Msi->MessageControl.MsiEnable = 0;
					PciDeviceConfigurationWrite16(Device, *(UINT16*)&Msi->MessageControl, CapOffset + MSI_MESSAGE_CONTROL);

					IRQ_CONTROL_DESCRIPTOR* Icd = RegisterIrq(0, Handler, IRQ_SOURCE_MESSAGE_SIGNALED_INTERRUPT, 0);
					if(!Icd) return KERNEL_SERR_OUT_OF_RANGE;
					
					Icd->Device = Device;
					Icd->Driver = Device->Driver;
					Icd->Source = INTERRUPT_SOURCE_IRQMSI;

					UINT8 MsiVector = Icd->InterruptVector;
					if(Msi->MessageControl.x64AddressCapable) {
						PCI_MSI_CAPABILITY_64BIT* MsiX64 = (PCI_MSI_CAPABILITY_64BIT*)CapBuffer;
						MsiX64->MessageData = MsiVector | (1 << 14); // Assert
						MsiX64->MessageAddress = (UINT64)LocalApicPhysicalAddress + (Icd->LapicId << 12);
						MsiX64->MessageControl.MsiEnable = 1;
						MsiX64->Mask = 0;
						MsiX64->MessageControl.MultipleMessageEnable = 0;
                    // SystemDebugPrint(L"PCI : MSI64 Capability MSG_CONTROL : %x, MessageAddress : %x, MSG_DATA : %x", (UINT64)*(UINT16*)&MsiX64->MessageControl, (UINT64)MsiX64->MessageAddress, (UINT64)MsiX64->MessageData);
						PciDeviceConfigurationWrite64(Device, MsiX64->MessageAddress, CapOffset + MSI_MESSAGE_ADDRESS);
						PciDeviceConfigurationWrite16(Device, MsiX64->MessageData, CapOffset + MSI64_MESSAGE_DATA);
						PciDeviceConfigurationWrite16(Device, *(UINT16*)&MsiX64->MessageControl, CapOffset + MSI_MESSAGE_CONTROL);
					} else {

						Msi->MessageData = MsiVector | (1 << 14); // Assert
						Msi->MessageAddress = (UINT64)LocalApicPhysicalAddress + (Icd->LapicId << 12);
						Msi->MessageControl.MsiEnable = 1;
						Msi->Mask = 0;
						Msi->MessageControl.MultipleMessageEnable = 0;

                    // SystemDebugPrint(L"PCI : MSI Capability MSG_CONTROL : %x, MessageAddress : %x, MSG_DATA : %x", (UINT64)*(UINT16*)&Msi->MessageControl, (UINT64)Msi->MessageAddress, (UINT64)Msi->MessageData);

						PciDeviceConfigurationWrite32(Device, Msi->MessageAddress, CapOffset + MSI_MESSAGE_ADDRESS);
						PciDeviceConfigurationWrite16(Device, Msi->MessageData, CapOffset + MSI_MESSAGE_DATA);
						PciDeviceConfigurationWrite16(Device, *(UINT16*)&Msi->MessageControl, CapOffset + MSI_MESSAGE_CONTROL);

					}

					
					SystemDebugPrint(L"Registered IRQ : %d, APIC_ID : %d", (UINT64)Icd->InterruptVector, (UINT64)Icd->LapicId);
					// while(1);
	
					SZeroMemory(Msi);
                    for(UINT i = 0;i<sizeof(PCI_MSI_CAPABILITY_64BIT) >> 2;i++){
                        CapBuffer[i] = PciDeviceConfigurationRead32(Device, CapOffset + (i << 2));
                    }
                    
                    if(Msi->MessageControl.x64AddressCapable) {
						PCI_MSI_CAPABILITY_64BIT* MsiX64 = (PCI_MSI_CAPABILITY_64BIT*)CapBuffer;
                    // SystemDebugPrint(L"PCI (RESULT) : MSI64 Capability MSG_CONTROL : %x, MessageAddress : %x, MSG_DATA : %x", (UINT64)*(UINT16*)&MsiX64->MessageControl, (UINT64)MsiX64->MessageAddress, (UINT64)MsiX64->MessageData);

					} else {
                    // SystemDebugPrint(L"PCI (RESULT) : MSI Capability MSG_CONTROL : %x, MessageAddress : %x, MSG_DATA : %x", (UINT64)*(UINT16*)&Msi->MessageControl, (UINT64)Msi->MessageAddress, (UINT64)Msi->MessageData);

					}
					// SystemDebugPrint(L"IRQ%d HANDLER: %x", (UINT64)Icd->InterruptVector, Icd->Handler);
					return KERNEL_SOK;
                } else if(CapabilityHeader->CapabilityId == PCI_CAPABILITY_ID_MSI_X) {
					SystemDebugPrint(L"MSI-X Supported");
					while(1);
				}


				
                if(!CapabilityHeader->NextPointer) break;
                CapOffset = CapabilityHeader->NextPointer;
                _caphdr = PciDeviceConfigurationRead16(Device, CapOffset);
            }
        }



	return KERNEL_SOK;
}