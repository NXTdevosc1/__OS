; GLOBAL OS DIGITAL SIGNATURE
[BITS 64]
; GLBDSIG Structure:
	; UINT16 OsMajorVersion
	; UINT16 OsMinorVersion
	; UINT16 BootloaderMinVersion
	; UINT16 UefiMajorVersion
	; UINT16 UefiMinorVersion
	; CHAR   OsName[20]
	; 
; ---------------------------

global KrnlEntry
global InitData

extern _start
; Kernel Entry
; Sets Kernel Stack Pointer
; Clear RFLAGS. Cause of some flags set by bootloader
; Like Nested Task Flag set from the call to the entry point

section .text
	KrnlEntry:

		mov rsp, _KernelStackTop
		mov rbp, rsp
		sub rsp, 0x1000
		add rbp, 0x800
		pushfq
		pop rax
		xor rax, rax
		mov cr8, rax ; Set Task Priority Register to 0
		push rax
		popfq
		
		; Enable SSE (Required)
		mov rax, cr0
		and rax, ~(1<<2) ; Clear emulation bit
		or rax, 2 ; Set Monitor COPROCESSOR Bit
		mov cr0, rax
		mov rax, cr4
		or rax, 3 << 9 ; Set OSFXSR, OSXMMEXCPT bits
		or rax, 1 << 7 ; Enable Global Pages
		mov cr4, rax
		
		; Check NX Compatiblity

		mov eax, 0x80000001
		cpuid
		test edx, 1 << 20
		jz .NoNX

		mov ecx, 0xC0000080 ; IA32_EFER
		rdmsr
		mov ecx, 0xC0000080
		or eax, 1 << 11 ; Enable NX Bit
		xor edx, edx
		wrmsr


		.NoNX:
		add rsp, 8 ; Compiler expects the unaligned stack setup : RCX, RDX, R8, R9, RIP (Return Address)
		jmp _start




extern SystemSpaceBase
extern KeGlobalCR3

global KernelRelocate
extern __KernelRelocate
extern _kmain

align 0x1000
KernelRelocate:

	mov rax, InitData

	; Preserve address of InitData, _KernelStackTop
	mov r8, rax
	mov r9, [rel KeGlobalCR3]
	mov rax, [rax + 0x3C]
	push rax
	push r8
	push r9
	sub rsp, 0x20 ; Callee stack
	call __KernelRelocate
	add rsp, 0x20
	pop r9
	pop r8
	pop rax
	wbinvd



	; Calculate new rsp
	mov rcx, r8
	mov rcx, [rcx + 0x3C]
	sub rsp, rax
	add rsp, rcx
	

	mov cr3, r9
	mov rdx, _KernelStackTop
	mov rbp, rdx ; Base Pointer = Stack Top
	
	push 0
	popf

	mov rax, _kmain
	jmp rax

global __JumpToRelocatedKernel



	
	


section .GLBDSIG
	dw 1
	dw 0
	dw 1
	dw 2
	dw 0
	OSNAME:
	db 'OS [Not Ready]'
	times 20 - ($ - OSNAME) db 0


; RESERVE 4K For Bootloader INITDATA
section INITDATA
	InitData times 0x1000 db 0


; RESERVED 4K For Bootloader Payload 
section .PRTVRFY
	times 0x1000 db 0

section .KSTACK

align 0x1000
	times 0x1000 db 0 ; Protective Padding
	_KernelStackBottom:
	times 0x8000 dq 0
	_KernelStackTop:
	times 0x1000 db 0 ; Protective Padding

