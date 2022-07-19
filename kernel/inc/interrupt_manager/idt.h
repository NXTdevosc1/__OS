#pragma once
#include <stdint.h>
#include <IO/utility.h>
#include <interrupt_manager/interrupts.h>
#include <Management/device.h>

#define CPU_INTERRUPT_DIVIDED_BY_0 0
#define CPU_INTERRUPT_DEBUG_EXCEPTION 1
#define CPU_INTERRUPT_NON_MASKABLE_INTERRUPT 2
#define CPU_INTERRUPT_BREAK_POINT 3
#define CPU_INTERRUPT_INTO_DETECTED_OVERFLOW 4
#define CPU_INTERRUPT_OUT_OF_BOUNDS_EXCEPTION 5
#define CPU_INTERRUPT_INVALID_OPCODE_EXCEPTION 6
#define CPU_INTERRUPT_NO_COPROCESSOR_EXCEPTION 7
#define CPU_INTERRUPT_DOUBLE_FAULT 8
#define CPU_INTERRUPT_COPROCESSOR_SEGMENT_OVERRUN 9
#define CPU_INTERRUPT_INVALID_TSS 10
#define CPU_INTERRUPT_SEGMENT_NOT_PRESENT 11
#define CPU_INTERRUPT_STACK_FAULT 12
#define CPU_INTERRUPT_GENERAL_PROTECTION_FAULT 13
#define CPU_INTERRUPT_PAGE_FAULT 14
#define CPU_INTERRUPT_UNKOWN_INTERRUPT_EXCEPTION 15
#define CPU_INTERRUPT_COPROCESSOR_FAULT 16
#define CPU_INTERRUPT_ALIGNMENT_CHECK_EXCEPTION 17
#define CPU_INTERRUPT_MACHINE_CHECK_EXCEPTION 18
#define CPU_INTERRUPT_SIMD_FLOATING_POINT_EXCEPTION 19


#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */
#define PIC_EOI		0x20		/* End-of-interrupt command code */




#define INTERRUPT_SOURCE_USER 0 // User accessible interrupt
#define INTERRUPT_SOURCE_IRQ 1
#define INTERRUPT_SOURCE_IRQMSI 2
#define INTERRUPT_SOURCE_SYSTEM 3 // Like multiprocessor interrupts






#pragma pack(push, 1)

enum IDT_DESCRIPTOR_TYPE{
    IDT_CALL_GATE = 12,
    IDT_INTERRUPT_GATE = 14,
    IDT_TRAP_GATE = 15
};

struct IDT_ENTRY{
   uint16_t offset_low;
   uint16_t segment_selector;
   uint8_t ist : 3;
   uint8_t reserved0 : 5;
   uint8_t type : 4;
   uint8_t reserved1 : 1;
   uint8_t dpl : 2;
   uint8_t present : 1;
   uint16_t offset_mid;
   uint32_t offset_high;
   uint32_t reserved2;
};
#pragma pack(push , 1)


typedef struct _INTERRUPT_INFORMATION {
    RFDEVICE_OBJECT Device;
    UINT32 Source;
    void* PreviousThread; // Mainly used in interrupt_source_user
    void* InterruptStack;
} INTERRUPT_INFORMATION, *RFINTERRUPT_INFORMATION;

typedef void (__cdecl* KEIRQ_ROUTINE)(RFDRIVER_OBJECT DriverObject, RFINTERRUPT_INFORMATION InterruptInformation);


typedef struct _IRQ_CONTROL_DESCRIPTOR {
    UINT8 Present;
    UINT8 VirtualIoApicId;
    UINT8 PhysicalIrqNumber;
    UINT8 IoApicIrqNumber; // index to IRQ Redirection Table
    UINT Flags;
    void* Process;
    KEIRQ_ROUTINE Handler;
    UINT8 InterruptVector; // Set to 0x20 + IrqIndex
    UINT32 LapicId;
    UINT Source;
    RFDEVICE_OBJECT Device;
    RFDRIVER_OBJECT Driver;
    INTERRUPT_INFORMATION InterruptInformation; // Buffer sent to the driver
    char Reserved[51]; // to align at 128 bytes
} IRQ_CONTROL_DESCRIPTOR;



#pragma pack(pop)



uint64_t IDT_get_offset(struct IDT_ENTRY* entry);
void SetInterruptGate(void* handler, uint8_t offset, uint8_t dpl, uint8_t ist, uint8_t type, uint16_t cs);

void KERNELAPI RegisterInterruptServiceRoutine(INTERRUPT_SERVICE_ROUTINE Handler, UINT8 Offset);


enum _IRQ_DELIVERY_MODE{
    IRQ_DELIVERY_NORMAL = 0,
    IRQ_DELIVERY_SMI,
    IRQ_DELIVERY_NMI,
    IRQ_DELIVERY_EXTINT
};

typedef enum _IRQ_CONTROL_FLAGS{
    IRQ_CONTROL_DISABLE_OVERRIDE_CHECK = 1,
    IRQ_CONTROL_LOW_ACTIVE = 2,
    IRQ_CONTROL_EDGE_TRIGGERED = 4,
    IRQ_CONTROL_DISABLE_OVERRIDE_FLAGS = 8,
    IRQ_SOURCE_MESSAGE_SIGNALED_INTERRUPT = 0x10,
    IRQ_CONTROL_USE_BASIC_INTERRUPT_WRAPPER = 0x20 // Interrupt wrapper providing minimal information, and not preempting
} IRQ_CONTROL_FLAGS;



KERNELSTATUS KERNELAPI KeControlIrq(KEIRQ_ROUTINE Handler, UINT IrqNumber, UINT DeliveryMode, UINT Flags);
KERNELSTATUS KERNELAPI KeReleaseIrq(UINT IrqNumber);

KERNELSTATUS KERNELAPI SetInterruptService(RFDEVICE_OBJECT Device, KEIRQ_ROUTINE Handler);

struct IDTR{
    uint16_t limit;
    uint64_t offset;
};

struct INTERRUPT_DESCRIPTOR_TABLE
{
    struct IDT_ENTRY entries[256];
};
#pragma pack(pop)

void GlobalInterruptDescriptorInitialize();
void GlobalInterruptDescriptorLoad();
void remap_pic();

extern void exit_interrupt();

extern void* GlobalWrapperPointer[];
extern void* GlobalIsrPointer[];

extern void* gIRQCtrlWrapPtr[];

extern struct INTERRUPT_DESCRIPTOR_TABLE KERNEL_IDT;
extern struct IDTR idtr;