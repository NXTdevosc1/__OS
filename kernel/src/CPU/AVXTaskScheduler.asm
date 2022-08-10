
[BITS 64]

%include "src/CPU/schedulerdefs.asm"

section .text

; TODO : VPINSRQ To mov registers across YMM (Result : 4/3 times faster register switching)
global SchedulerEntryAVX

SchedulerEntryAVX:
    cli
    push rdx
	push rcx
	push rbx
	push rax
    
    mov rax, [rel SystemSpaceBase]
    mov ebx, [rax + APIC_ID]
    shr ebx, 24
    shl rbx, CPU_MGMT_SHIFT
    lea rax, [rbx + SYSTEM_SPACE_CPUMGMT + rbx]
    mov rbx, [rax + CPM_CURRENT_THREAD]
    jmp .AVXSaveRegisters
.R0:
    jmp $
.AVXSaveRegisters:
    mov rcx, [rbx + _XSAVE]
    xsave [rcx]
    ; Pushed registers
    vmovdqu ymm0, [rsp]
    vmovdqu [rbx + __MM32_RAXRBXRCXRDX], ymm0
    ; Save Interrupt registers
    vmovdqu ymm0, [rsp + 0x20]
    vmovdqu [rbx + __MM32_RIPCSRFLAGSRSP], ymm0
    ; Segment Registers
	mov cx, fs
	shl rcx, 16
	mov cx, gs
	shl rcx, 16
	mov cx, [rsp + 0x40]
	shl rcx, 16
	mov cx, ds

	mov [rbx + _DS], rcx ; DS, SS, GS, FS
	mov [rbx + _ES], es
    ; Remaining Registers
    movq xmm1, rdi
    movq xmm2, rsi
    vpunpcklqdq xmm0, xmm1, xmm2
    vpslldq ymm0, 16
    movq xmm1, r8
    movq xmm2, r9
    jmp .R0