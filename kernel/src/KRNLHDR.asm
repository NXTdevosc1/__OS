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

	

		jmp _start

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
	InitData:
	times 0x1000 db 0xE5


; RESERVED 4K For Bootloader Payload 
section .PRTVRFY
	times 0x1000 db 0xC7

section .KSTACK
	times 0x1000 db 0 ; Protective Padding
	_KernelStackBottom:
	times 0x5000 dq 0
	_KernelStackTop:
	times 0x1000 db 0 ; Protective Padding

