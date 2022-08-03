
[BITS 64]

extern CpuManagementTable
global gIRQCtrlWrapPtr
extern SystemSpaceBase
extern InterruptUnsupported



NUM_IRQS_PER_PROCESSOR equ 24
SIZE_BITSHIFT_IRQ_CTRLTBL equ 7 ; 128 Bytes


IRQ_DEVICE_DRIVER equ 0x29
IRQ_INTERRUPT_INFORMATION equ 0x31

IRQ_DEVICE equ 0x21
IRQ_SOURCE equ 0x1D

INTINF_DEVICE equ 0
INTINF_SOURCE equ 8
INTINF_PREVIOUS_THREAD equ 0x10
INTINF_INT_STACK equ 0x18

extern SkipTaskSchedule

%include "src/CPU/schedulerdefs.asm"


extern SchedulerEntrySSE

section .text

%macro DeclareIRQControlWrapper 1


align 0x10
IRQControlWrapper%1:
; Entering System Interrupts Process
    ; Standard scheduler entry (do not cli as the process rflags will be saved cli'd
    ; Task switch will not occur as APIC_EOI is not set
    ; this is why we jmp SchedulerEntry instead of int SCHEDULE
    
    cli
    push rax
    push rbx
    push rcx

    mov rax, [rel SystemSpaceBase]
    mov ebx, [rax + APIC_ID]
    shr ebx, 24
    shl rbx, CPU_MGMT_SHIFT
    lea rax, [rax + SYSTEM_SPACE_CPUMGMT + rbx]

    mov rbx, [rax + CPM_SYSTEM_INTERRUPTS_THREAD]
    mov [rax + CPM_FORCE_NEXT_THREAD], rbx
    
    ; Parameters
    mov rcx, [rax + CPM_CURRENT_THREAD]
    mov [rbx + _RDX], rcx

    lea rcx, [rsp + 0x18]
    mov [rbx + _RDI], rcx

    mov rax, .InterruptsThreadEntry
    mov [rbx + _RIP], rax
    ; Stack frame already built

    pop rcx
    pop rbx
    pop rax


    jmp SchedulerEntrySSE
    
    ; INT Schedule does the APIC_EOI For us
    ; jmp SkipTaskSchedule ; TODO : after task switching finish
align 0x10
.InterruptsThreadEntry: ; New code is being executed on last registers are discarded
    ; PARAMETERS : rdx = Previous Thread, rdi = Interrupt Stack
    ; CLI Already set on thread settings

    
    mov rsp, rdi ; IST 3
    mov rbp, rsp

    mov rax, [rel SystemSpaceBase]


    mov ebx, [rax + APIC_ID]
    shr rbx, 24
    shl rbx, CPU_MGMT_SHIFT ; mul 0x1000
    lea rax, [rax + SYSTEM_SPACE_CPUMGMT + rbx + CPM_IRQ_CONTROL_TABLE]

    mov rbx, %1
    shl rbx, SIZE_BITSHIFT_IRQ_CTRLTBL
    add rax, rbx

    ; Check presence of IRQ
    xor rcx, rcx
    mov cl, [rax + 24]
    cmp byte [rax], 1
    jne InterruptUnsupported

    ; Build Interrupt Information
    lea rsi, [rax + IRQ_INTERRUPT_INFORMATION]
    mov rbx, [rax + IRQ_DEVICE]
    mov [rsi + INTINF_DEVICE], rbx
    mov ebx, [rax + IRQ_SOURCE]
    mov [rsi + INTINF_SOURCE], ebx
    mov [rsi + INTINF_PREVIOUS_THREAD], rdx ; PASSED_PARAMETER
    mov [rsi + INTINF_INT_STACK], rdi ; PASSED_PARAMETER

    ; Build Parameters
    mov rcx, [rax + IRQ_DEVICE_DRIVER]
    mov rdx, rsi
    push rcx
    push rdx


    ; Call Handler
    mov rbx, [rax + 0x10]

    call rbx

    int 0x40

    mov rax, 0xbeef
    jmp $

    mov rax, OverflowCheck
    mov rbx, OverflowCheckStackTop

    int 0x40

    jmp OverflowCheck


%endmacro

OverflowCheck:
    cli
    mov rax, 0xDADBEEF
    hlt
    jmp OverflowCheck

OverflowCheckStack times 0x200 db 0
OverflowCheckStackTop:

%assign i 0
%rep 0x100
    DeclareIRQControlWrapper i
%assign i i+1
%endrep

%macro IRQCWPtr 1
dq IRQControlWrapper%1
%endmacro


section .data

SchedullingWasDisabled dq 0

align 0x1000
gIRQCtrlWrapPtr:
    %assign i 0
    %rep 0x100
        IRQCWPtr i
    %assign i i+1
    %endrep