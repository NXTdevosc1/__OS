#include <interrupt_manager/interrupts.h>
#include <interrupt_manager/SOD.h>
#include <input/keyboard.h>
#include <IO/utility.h>
#include <interrupt_manager/idt.h>
#include <preos_renderer.h>
#include <input/mouse.h>
#include <CPU/process.h>
#include <CPU/cpu.h>
#include <cstr.h>
#include <kernel.h>
#include <CPU/cpu.h>

extern void InterruptUnsupported(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame) {
	TaskSchedulerDisable();
	__cli();
	char Msg[100] = "Unsupported Interrupt Number : ";

	UINT64 Div = 1;
	UINT64 Reminder = 10;
	UINT64 CharacterIndex = 31;
	UINT64 CharacterCount = 1;
	UINT64 Tmp = InterruptNumber;
	while ((Tmp /= 10)) Div *= 10; Reminder *= 10; CharacterCount++;

	for (UINT64 i = 0;i<CharacterCount;i++, CharacterIndex++) {
		if (!((InterruptNumber / Div) % Reminder)) break;
		Msg[CharacterIndex] = '0' + (InterruptNumber / Div) % Reminder;
		Div /= 10;
		Reminder /= 10;
	}
	Msg[CharacterIndex] = 0;

	SOD(0, Msg);
	for (;;) __hlt();
}

void INTH_SIMD_FLOATING_POINT_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame) {
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_SIMD_FLOATING_POINT_EXCEPTION, "SIMD FLOATING POINT EXCEPTION");
	__hlt();
	while (1);
}

void INTH_DIVIDED_BY_0(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_DIVIDED_BY_0,"DIVIDED BY 0");
	__hlt();
	while(1);

}
void INTH_DEBUG_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	
	SOD(CPU_INTERRUPT_DEBUG_EXCEPTION,"DEBUG FAULT");
	__hlt();
	while(1);

}
void INTH_NON_MASKABLE_INTERRUPT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	// __setCR3(KeGlobalCR3);
	_RT_SystemDebugPrint(L"NMI");
	UINT8 SystemControlPortA = InPortB(SYSTEM_CONTROL_PORT_A);
	UINT8 SystemControlPortB = InPortB(SYSTEM_CONTROL_PORT_B);

	if(SystemControlPortA & (1 << 4)) {
		_RT_SystemDebugPrint(L"NMI : WATCH DOG TIMER STATUS");
	}
	if(SystemControlPortB & (1 << 6)) {
		_RT_SystemDebugPrint(L"NMI : CHANNEL CHECK");
	}
	if(SystemControlPortB & (1 << 7)) {
		_RT_SystemDebugPrint(L"NMI : PARITY CHECK");
	}

	// SOD(CPU_INTERRUPT_NON_MASKABLE_INTERRUPT,"NON MASKABLE INTERRUPT");
	// __hlt();
	// while(1);

}
void INTH_BREAK_POINT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	
	SOD(CPU_INTERRUPT_BREAK_POINT,"BREAK POINT EXCEPTION");
	__hlt();
	while(1);

}
void INTH_INTO_DETECTED_OVERFLOW(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_INTO_DETECTED_OVERFLOW,"INTO DETECTED OVERFLOW EXCEPTION");
	__hlt();
	while(1);

}
void INTH_OUT_OF_BOUNDS_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_OUT_OF_BOUNDS_EXCEPTION,"OUT OF BOUNDS EXCEPTION");
	__hlt();
	while(1);

}
void INTH_INVALID_OPCODE_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	//__setCR3(KeGlobalCR3);
	GP_clear_screen(0);
	GP_draw_sf_text("Invalid Opcode Exception In Address :", 0xfffffff, 20, 20);
	GP_draw_sf_text(to_hstring64(InterruptFrame->IntStack.InstructionPointer), 0xfffffff, 20, 40);

	//SOD(CPU_INTERRUPT_INVALID_OPCODE_EXCEPTION,"INVALID OPCODE EXCEPTION");
	__hlt();
	while(1);

}
void INTH_NO_COPROCESSOR_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_COPROCESSOR_FAULT,"NO COPROCESSOR EXCEPTION");
	__hlt();
	while(1);

}
void INTH_INVALID_TSS(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_INVALID_TSS,"INVALID TSS");
	__hlt();
	while(1);

}
void INTH_SEGMENT_NOT_PRESENT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_SEGMENT_NOT_PRESENT,"SEGMENT NOT PRESENT");
	__hlt();
	while(1);

}
void INTH_STACK_FAULT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_STACK_FAULT,"STACK FAULT");
	__hlt();
	while(1);

}
void INTH_GENERAL_PROTECTION_FAULT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_GENERAL_PROTECTION_FAULT,"GENERAL PROTECTION FAULT");
	__hlt();
	while(1);

}
extern void SchedulerEntrySSE();

void INTH_PAGE_FAULT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	struct TSS_ENTRY* CpuTSS = (struct TSS_ENTRY*)((UINT64)CpuManagementTable[0]->CpuBuffer + CPU_BUFFER_TSS_BASE);
	_RT_SystemDebugPrint(L"TSCH : %x , INSTRUCTION : %x, IST2 : %x", SchedulerEntrySSE, InterruptFrame->IntStack.InstructionPointer, CpuTSS->ist2);
	while(1);
	SOD(CPU_INTERRUPT_PAGE_FAULT,"PAGE FAULT");
	__hlt();
	while(1);

}
void INTH_UNKNOWN_INTERRUPT_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_UNKOWN_INTERRUPT_EXCEPTION,"UNKNOWN INTERRUPT EXCEPTION");
	__hlt();
	while(1);

}
void INTH_COPROCESSOR_FAULT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_COPROCESSOR_FAULT,"COPROCESSOR FAULT");
	__hlt();
	while(1);

}
void INTH_ALIGNMENT_CHECK_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_ALIGNMENT_CHECK_EXCEPTION,"ALIGNMENT CHECK EXCEPTION");
	__hlt();
	while(1);

}
void INTH_MACHINE_CHECK_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_MACHINE_CHECK_EXCEPTION,"MACHINE CHECK EXCEPTION");
	__hlt();
	while(1);

}




void INTH_DBL_FAULT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	__cli();
	__setCR3(KeGlobalCR3);
	SOD(CPU_INTERRUPT_DOUBLE_FAULT,"DOUBLE FAULT");
	__hlt();
	while(1);

}


void INTH_keyboard(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){	handle_keyboard(InPortB(0x60));
}
void INTH_mouse(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame){
	handle_mouse_input(InPortB(0x60));
}
