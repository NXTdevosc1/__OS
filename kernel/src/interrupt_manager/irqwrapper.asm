
[BITS 64]

extern CpuManagementTable
global gIRQCtrlWrapPtr
extern SystemSpaceBase
extern InterruptUnsupported

NUM_IRQS_PER_PROCESSOR equ 24
SIZE_BITSHIFT_IRQ_CTRLTBL equ 7 ; 128 Bytes

SYSTEM_SPACE_CPUMGMT equ 0x200000
CPM_NEXT_THREAD		equ 0x100
CPM_INTERRUPTS_THREAD equ 0x108
CPM_IRQ_CONTROL_TABLE equ 0x110

TH_REGISTERS equ 0x4C
TH_RIP equ TH_REGISTERS + 0x40

APIC_ID		equ 0x20
APIC_EOI equ 0xB0

IRQ_DEVICE_DRIVER equ 0x29
IRQ_INTERRUPT_INFORMATION equ 0x31

IRQ_DEVICE equ 0x21
IRQ_SOURCE equ 0x1D

INTINF_DEVICE equ 0
INTINF_SOURCE equ 8
INTINF_PREVIOUS_THREAD equ 0x10
INTINF_INT_STACK equ 0x18

extern SkipTaskSchedule

CPM_THREAD equ 0x08
TH_RDX equ TH_REGISTERS + 0x18
TH_RDI equ TH_REGISTERS + 0x20

CPU_MGMT_SHIFT equ 15

section .text

%macro DeclareIRQControlWrapper 1
align 0x10
IRQControlWrapper%1:
; Entering System Interrupts Process
    cli
    push rax
	push rbx
    push rcx
	mov rax, [rel SystemSpaceBase]

	mov ebx, [rax + APIC_ID]
	shr ebx, 24
	shl rbx, CPU_MGMT_SHIFT ; Multiply by 1000
	lea rax, [rax + SYSTEM_SPACE_CPUMGMT + rbx]
	mov rbx, [rax + CPM_INTERRUPTS_THREAD]
    mov [rax + CPM_NEXT_THREAD], rbx
    mov rcx, .InterruptsThreadEntry
	mov [rbx + TH_RIP], rcx ; Interrupt Handler Wrapper
	mov rcx, [rax + CPM_THREAD]
    mov [rbx + TH_RDX], rcx
    lea rcx, [rsp + 0x18] ; INT_STACK
    mov [rbx + TH_RDI], rcx
    


	pop rcx
    pop rbx
	pop rax

    
    
    ; INT Schedule does the APIC_EOI For us
    jmp SkipTaskSchedule
align 0x10
.InterruptsThreadEntry: ; New code is being executed on last registers are discarded
    ; PARAMETERS : rdx = Previous Thread, rdi = Interrupt Stack
    cli
    

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
    mov rax, [rel SystemSpaceBase]
	; mov dword [rax + 0xB0], 0


    ; Build Stack Frame
    mov rax, OverflowCheckStackTop
    mov rbx, OverflowCheck ; Set overflow check (Interrupt thread must not be re-called)

    int 0x9F
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