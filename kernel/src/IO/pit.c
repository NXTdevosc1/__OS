#include <IO/pit.h>
#include <IO/utility.h>
#include <interrupt_manager/SOD.h>
#include <preos_renderer.h>
#include <cstr.h>
#include <acpi/madt.h>
UINT64 PitCounter = 0;
UINT PitInterruptSource = 0; // PIT IRQ, May be modified and declared in the ACPI
UINT PitSeconds = 0;
UINT TmpPitCounter = 0;

// 18HZ * 2,7462 (49,9986903 HZ)
// FINAL (smally modified value) 0x5D38

void PitInterruptHandler(RFDRIVER_OBJECT DriverObject, RFINTERRUPT_INFORMATION InterruptInformation){
    // GP_draw_sf_text(to_stringu64(PitCounter - 1), 0, 20, 20);
    // GP_draw_sf_text(to_stringu64(PitCounter), 0xffffff, 20, 20);
    
    PitCounter++;
    TmpPitCounter++;

    if(TmpPitCounter == 50){
        PitSeconds++;
        TmpPitCounter = 0;
        GP_draw_sf_text(to_stringu64(PitSeconds - 1), 0, 20, 400);
        GP_draw_sf_text(to_stringu64(PitSeconds), 0xffffff, 20, 400);
    }
}

void PitEnable(){
    OutPortB(PIT_COMMAND, 0b00110100); // Select count register at channel 0

    // Set a 100HZ Frequency
    UINT16 Divisor = 11930;
    OutPortB(PIT_CHANNEL0, Divisor); // Count Low
    OutPortB(PIT_CHANNEL0, Divisor >> 8); // Count High
    
    if(KeControlIrq(PitInterruptHandler, 0 /*IRQ will be redirected if there is a redirection entry, probably IRQ 2*/, IRQ_DELIVERY_NORMAL, IRQ_CONTROL_USE_BASIC_INTERRUPT_WRAPPER) != KERNEL_SOK) SET_SOD_INITIALIZATION;
    UINT32 Flags = 0;
    UINT32 IRQ = 0; // Pit is the first interrupt to be set by kernel on CPU 0
    // GP_clear_screen(0);
}
void PitDisable(){
    // Set to the slowest availaible count
    OutPortB(PIT_COMMAND, 0b00110100); // Select count register at channel 0

    // Set a 50HZ Frequency
    OutPortB(PIT_CHANNEL0, 0xff); // Count Low
    OutPortB(PIT_CHANNEL0, 0xff); // Count High

    // Mask PIT IRQ
    KeReleaseIrq(0);
}
void PitWait(DWORD Clocks){
    // Generally the APIC Timer will wait 1 clock to equalize Counter
    // then it will wait for 5 clocks to get 0.1 second time
    UINT64 WaitUntil = PitCounter + Clocks;
    while(PitCounter < WaitUntil) __hlt();
}