// PROGRAMMABLE INTERRUPT CONTROLLER (Basic Driver) to use more precise timers like APIC Timer
#pragma once
#include <krnltypes.h>
#include <CPU/cpu.h>

// PIT IO PORTS
#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42

#define PIT_COMMAND 0x43

/*
PIT COMMAND :
Bit 0 - Binary/BCD Mode (0 = Binary)
Bits 3:1 - Operating Mode
Bits 5:4 - Access Mode (0 = latch count value, 1 = lobye, 2 = hibyte, 3 = lobyte/hibyte)
Bits 7:6 - Select Channel
*/

// PIT will operate on 50HZ Speed (best found speed for the highest precision)

void PitEnable(void);
void PitDisable(void);
void PitInterruptHandler(RFDRIVER_OBJECT DriverObject, RFINTERRUPT_INFORMATION InterruptInformation);
void PitWait(DWORD Clocks);

extern UINT PitInterruptSource;