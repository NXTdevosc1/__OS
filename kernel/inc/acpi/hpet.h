#pragma once
#include <acpi/acpi_defs.h>
#include <krnltypes.h>
#pragma pack(push, 1)
typedef struct _ACPI_HPET{
    ACPI_SDT Sdt;
    UINT8 HardwareRevisionId;
    UINT8 ComparatorCounter : 5;
    UINT8 CounterSize : 1;
    UINT8 Reserved0 : 1;
    UINT8 LegacyReplacement : 1;
    UINT16 PciVendorId;
    ACPI_GENERIC_ADDRESS_STRUCTURE Address;
    UINT8 HpetNumber;
    UINT16 MinimumTick;
    UINT8 PageProtection;
} ACPI_HPET;

typedef struct {
    UINT64 GeneralCapabilitiesAndId;
    UINT64 Reserved0;
    UINT64 GeneralConfiguration;
    UINT64 Reserved1;
    // GENERAL_INT_STATUS : Timers 0-31 INTSTS bitmask
    UINT64 GeneralInterruptStatus;
    UINT8 Reserved2[0xF0-0x28];
    UINT64 MainCounterValue;
    UINT64 Reserved3;
    UINT64 Timer0ConfigurationAndCapability;
    UINT64 Timer0ComparatorValue;
    UINT64 Timer0FsbInterruptRoute;
    UINT64 Reserved4;
    UINT64 Timer1ConfigurationAndCapability;
    UINT64 Timer1ComparatorValue;
    UINT64 Timer1FsbInterruptRoute;
    UINT64 Reserved5;
    UINT64 Timer2ConfigurationAndCapability;
    UINT64 Timer2ComparatorValue;
    UINT64 Timer2FsbInterruptRoute;
    UINT64 Reserved6;
    struct {
        UINT64 TimerConfigurationAndCapability;
        UINT64 TimerComparatorValue;
        UINT64 TimerFsbInterruptRoute;
        UINT64 Reserved;
    } Timers[];// Reserved for timers 3-31
} HPET_REGISTERS;

#pragma pack(pop)

#ifdef __HPET_SRC
// General Capability and ID
    #define HPET_REVISION_ID 0 // Bit Offset
    #define HPET_NUM_TIMERS 8
    #define HPET_COUNT_SIZE 13
    #define HPET_LEGACY_REPLACEMENT_ROUTE 15
    #define HPET_VENDORID 16
    #define HPET_COUNTER_CLICK_PERIOD 32
// General Configuration
#define HPET_ENABLE 1
#define HPET_LEGACY_REPLACEMENT_ROUTE_ENABLE 2

// Timer(n) Config & Cap (Bit Offsets)
#define TIMER_INT_TYPE 1
#define TIMER_INT_ENABLE 2
#define TIMER_TYPE_CONFIG 3
#define TIMER_PERIODIC_INT_CAPABILITY 4
#define TIMER_SIZE 5 // 0 = 32 BITS / 1 = 64 BITS
#define TIMER_VALUE_SET 6
#define TIMER_32BIT_MODE_ENABLE 8
#define TIMER_INTERRUPT_ROUTE 9 // 5 Bits
#define TIMER_FSB_INTERRUPT_ENABLE 14
#define TIMER_FSB_DELIVERY_CAPABILITY 15
#define TIMER_INTERRUPT_ROUTING_CAPABILILTY 32
#endif

void HpetInitialize(ACPI_HPET* Hpet);
// Return value : HPET Presence 1 = Present
BOOL HpetConfigure();


UINT64 GetHighPrecisionTimeSinceBoot();
UINT64 GetHighPerformanceTimerFrequency();