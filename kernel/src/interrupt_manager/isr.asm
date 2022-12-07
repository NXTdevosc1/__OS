[BITS 64]

global GlobalWrapperPointer
global GlobalIsrPointer

extern InterruptUnsupported


extern SystemSpaceBase


section .text

%macro DeclareWrapper 1

InterruptServiceWrapper%1:
	cli
	push rdx
	lea rdx, [rsp + 8] ; Interrupt Stack Frame
	push rax
	push rbx
	push rcx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	mov rcx, %1


	mov rax, GlobalIsrPointer
	shl rcx, 3 ; Multiply by 8
	add rax, rcx
	mov rax, [rax]
	test rax, rax
	jz InterruptUnsupported

	call rax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rcx
	pop rbx
	; mov rax, [rel SystemSpaceBase]
	; mov dword [rax + 0xB0], 0 ; Signal EOI
	pop rax
	pop rdx
	iretq

%endmacro%

%macro DeclareWrapperPointer 1
	dq InterruptServiceWrapper%1
%endmacro




%assign i 0
%rep 0x100
	DeclareWrapper i
%assign i i+1
%endrep

section .data
align 0x1000
GlobalWrapperPointer:
	%assign i 0
%rep 0x100
	DeclareWrapperPointer i
%assign i i+1
%endrep

times 512 dq 0

section .bss

GlobalIsrPointer:
	resq 512