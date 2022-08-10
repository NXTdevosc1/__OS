#include <acpi/acpi.h>
#define __HPET_SRC
#include <acpi/hpet.h>
#include <Management/runtimesymbols.h>
#include <interrupt_manager/idt.h>
#include <Management/device.h>
#include <sys/drv.h>
#include <interrupt_manager/SOD.h>
#include <kernel.h>
#include <acpi/madt.h>
static BOOL HpetDetected = FALSE;

ACPI_HPET* gHpet = NULL;
static UINT64 HpetAddress = 0;

RFDEVICE_OBJECT HpetDevice = NULL;

static HPET_REGISTERS* Regs = NULL;
extern UINT64 HpetFrequency = 0;
UINT64 HpetFrequencyHigh = 0;

extern UINT64 HpetNumClocks = 0;
UINT64 HpetNumClocksHigh = 0;

extern UINT64 HpetTotalClockTime = 0; // Num Clocks * Frequency
extern UINT64 HpetMainCounterAddress = 0; // Used on task scheduler
// BOOL LegacyReplacementRoute = 

static UINT d = 0;
void HpetInterruptHandler(RFDRIVER_OBJECT Device, RFINTERRUPT_INFORMATION InterruptInformation) {
    // _RT_SystemDebugPrint(L"HPET Int #%d (Main Counter : %x) Time : %x | in ms : %d", HpetNumClocks, Regs->MainCounterValue, GetHighPrecisionTimeSinceBoot(), GetHighPrecisionTimeSinceBoot() / (HpetFrequency / 1000));
    HpetNumClocks++;
    HpetTotalClockTime += 0x1000;
    Regs->GeneralConfiguration &= ~HPET_ENABLE;
    // Regs->GeneralConfiguration
    Regs->MainCounterValue = 0;
    Regs->Timer0ComparatorValue = HpetFrequency;
    Regs->GeneralConfiguration |= HPET_ENABLE;
} 

UINT64 GetHighPrecisionTimeSinceBoot() {
    return HpetNumClocks * HpetFrequency + Regs->MainCounterValue;
}
UINT64 GetHighPerformanceTimerFrequency() {
    return HpetFrequency;
}

void HpetInitialize(ACPI_HPET* Hpet) {
    if(Hpet->Address.AddressSpace) return; // Only MMIO Access is supported
    if(HpetDetected) return; // Currently we are using only one HPET
    gHpet = Hpet;
    HpetAddress = gHpet->Address.Address;
    Regs = (HPET_REGISTERS*)HpetAddress;
    MapPhysicalPages(kproc->PageMap, Regs, Regs, 1, PM_MAP | PM_CACHE_DISABLE);
    if(!(Regs->GeneralCapabilitiesAndId & HPET_LEGACY_REPLACEMENT_ROUTE)) {
        // Legacy replacement is required
        return;
    }
    HpetDetected = 1;
    HpetDevice = InstallDevice(NULL, DEVICE_SOURCE_SYSTEM_RESERVED, (LPVOID)HpetAddress);
    if(!HpetDevice) SET_SOD_MEDIA_MANAGEMENT;
    SetDeviceDisplayName(HpetDevice, L"High Precision Event Timer (HPET)");

	_RT_SystemDebugPrint(L"High Precision Event Timer (HPET) Found.");
    
    Regs->GeneralConfiguration &= ~HPET_ENABLE;
    Regs->GeneralConfiguration |= HPET_LEGACY_REPLACEMENT_ROUTE_ENABLE;

    UINT AdditionalTimers = Regs->GeneralCapabilitiesAndId >> HPET_NUM_TIMERS;
    AdditionalTimers &= 0x1F;
    AdditionalTimers -= 2; // Not - 3 because NUM_TMR = Last timer which is NumTimers - 1
    _RT_SystemDebugPrint(L"Additional Timers : %d , Address Space : %x | Address : %x | Access Size : %x",AdditionalTimers, Hpet->Address.AddressSpace, Hpet->Address.Address, Hpet->Address.AccessSize);
    // Disable timers
    Regs->Timer0ConfigurationAndCapability &= ~(1 << TIMER_INT_ENABLE);
    Regs->Timer1ConfigurationAndCapability &= ~(1 << TIMER_INT_ENABLE);
    Regs->Timer2ConfigurationAndCapability &= ~(1 << TIMER_INT_ENABLE);
    // Set one shot mode
    Regs->Timer0ConfigurationAndCapability &= ~(1 << TIMER_TYPE_CONFIG);
    Regs->Timer1ConfigurationAndCapability &= ~(1 << TIMER_TYPE_CONFIG);
    Regs->Timer2ConfigurationAndCapability &= ~(1 << TIMER_TYPE_CONFIG);
    // Set edge senstive
    Regs->Timer0ConfigurationAndCapability &= ~(1 << TIMER_INT_TYPE);
    Regs->Timer1ConfigurationAndCapability &= ~(1 << TIMER_INT_TYPE);
    Regs->Timer2ConfigurationAndCapability &= ~(1 << TIMER_INT_TYPE);
    // Setting for additional timers
    for(UINT i = 0;i<AdditionalTimers;i++) {
        Regs->Timers[i].TimerConfigurationAndCapability &= ~(1 << TIMER_INT_ENABLE);
        Regs->Timers[i].TimerConfigurationAndCapability &= ~(1 << TIMER_TYPE_CONFIG);
        Regs->Timers[i].TimerConfigurationAndCapability &= ~(1 << TIMER_INT_TYPE);

    }

    HpetMainCounterAddress = (UINT64)&Regs->MainCounterValue;

}

// Return value : HPET Presence 1 = Present
BOOL HpetConfigure() {
    if(!gHpet) return FALSE;

    UINT32 IntMask = Regs->Timer0ConfigurationAndCapability >> TIMER_INTERRUPT_ROUTING_CAPABILILTY;
    UINT IntFlags = 0;
    UINT32 IntNum = GetRedirectedIrq(0, &IntFlags); // IRQ 0 In IOAPIC Input (Probably 2)

    UINT32 Cnf = Regs->Timer0ConfigurationAndCapability;
    Cnf &= ~(0x1F << TIMER_INTERRUPT_ROUTE);
    Cnf |= IntNum << TIMER_INTERRUPT_ROUTE;
    Regs->Timer0ConfigurationAndCapability = Cnf;
    if(KERNEL_ERROR(KeControlIrq(HpetInterruptHandler, IntNum, IRQ_DELIVERY_NORMAL, IRQ_CONTROL_DISABLE_OVERRIDE_CHECK))) SET_SOD_MEDIA_MANAGEMENT;


    
    _RT_SystemDebugPrint(L"HPET_SUCCESSFULLY_CONFIGURED (INT_ROUTE_CAP : %x , PERIOD : %x)", Regs->Timer0ConfigurationAndCapability >> TIMER_INTERRUPT_ROUTING_CAPABILILTY, Regs->GeneralCapabilitiesAndId >> HPET_COUNTER_CLICK_PERIOD);
    UINT64 Frequency = 0x38D7EA4C68000 / (Regs->GeneralCapabilitiesAndId >> HPET_COUNTER_CLICK_PERIOD);
    HpetFrequency = Frequency;
    _RT_SystemDebugPrint(L"Frequency : %d HZ", Frequency);
    Regs->MainCounterValue = 0;
    Regs->Timer0ComparatorValue = HpetFrequency;
    Regs->Timer0ConfigurationAndCapability |= (1 << TIMER_INT_ENABLE);
    Regs->GeneralConfiguration |= HPET_ENABLE;
    return TRUE;
}