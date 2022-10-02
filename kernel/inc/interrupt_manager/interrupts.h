#pragma once
#include <krnltypes.h>

// INTH reffers to : kernel interrupt_handler
//INTHU reffers to : user interrupt_handler
typedef union _INTERRUPT_STACK_FRAME {
    struct {
        UINT64	InstructionPointer;
        UINT64	CodeSegment;
        UINT64	CpuRflags;
        UINT64	StackPointer;
        UINT64	StackSegment;
    } IntStack;
    struct {
        UINT64 InterruptCode;
        UINT64	InstructionPointer;
        UINT64	CodeSegment;
        UINT64	CpuRflags;
        UINT64	StackPointer;
        UINT64	StackSegment;
    } CodeIntStack; // Interrupt stack passing code
} INTERRUPT_STACK_FRAME, *PINTERRUPT_STACK_FRAME;

typedef void(__cdecl* INTERRUPT_SERVICE_ROUTINE)(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptStackFrame);

extern void __InterruptCheckHalt(void);

extern void InterruptUnsupported(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);

void INTH_DIVIDED_BY_0(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_DEBUG_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_NON_MASKABLE_INTERRUPT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_BREAK_POINT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_INTO_DETECTED_OVERFLOW(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_OUT_OF_BOUNDS_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_INVALID_OPCODE_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_NO_COPROCESSOR_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_INVALID_TSS(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_SEGMENT_NOT_PRESENT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_STACK_FAULT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_GENERAL_PROTECTION_FAULT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_PAGE_FAULT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_UNKNOWN_INTERRUPT_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_COPROCESSOR_FAULT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_ALIGNMENT_CHECK_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_MACHINE_CHECK_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_SIMD_FLOATING_POINT_EXCEPTION(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);

void INTH_PIT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_DBL_FAULT(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_keyboard(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);
void INTH_mouse(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);

void INTH_IPI(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME InterruptFrame);


