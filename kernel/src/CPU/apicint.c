#include <CPU/cpu.h>
#include <interrupt_manager/SOD.h>

void ApicSpuriousInt(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf) {
	SOD(0, "APIC_SPURIOUS");
	SystemDebugPrint(L"APIC Spurious Interrupt");
	while(1);
}

void ApicThermalSensorInt(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf) {
    SystemDebugPrint(L"APIC_INT : TSR");
    *(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_END_OF_INTERRUPT) = 0;
}
void ApicPerformanceMonitorCountersInt(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf) {
    SystemDebugPrint(L"APIC_INT : PMCR");
    *(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_END_OF_INTERRUPT) = 0;
}
void ApicLint0Int(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf) {
    SystemDebugPrint(L"APIC_INT : LINT0");
    *(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_END_OF_INTERRUPT) = 0;
}
void ApicLint1Int(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf) {
    SystemDebugPrint(L"APIC_INT : LINT1");
    *(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_END_OF_INTERRUPT) = 0;
}
void ApicLvtErrorInt(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf) {
    SystemDebugPrint(L"APIC_INT : ERROR");
    *(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_END_OF_INTERRUPT) = 0;
}

void ApicCmciInt(UINT64 InterruptNumber, PINTERRUPT_STACK_FRAME Isf) {
    SystemDebugPrint(L"APIC_INT : CMCI");
    *(UINT32*)(LAPIC_ADDRESS + CPU_LAPIC_END_OF_INTERRUPT) = 0;
}