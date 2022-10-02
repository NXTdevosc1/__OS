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

	; A failed attempt to fix the Global Optimizations problem
		xor rax, rax
		xor rbx, rbx
		xor rcx, rcx
		xor rdx, rdx
		xor rsi, rsi
		xor rdi, rdi
		xor r8, r8
		xor r9, r9
		xor r10, r10
		xor r11, r11
		xor r12, r12
		xor r13, r13
		xor r14, r14
		xor r15, r15

		pxor xmm0, xmm0
		pxor xmm1, xmm1
		pxor xmm2, xmm2
		pxor xmm3, xmm3
		pxor xmm4, xmm4
		pxor xmm5, xmm5
		pxor xmm6, xmm6
		pxor xmm7, xmm7
		pxor xmm8, xmm8
		pxor xmm9, xmm9
		pxor xmm10, xmm10
		pxor xmm11, xmm11
		pxor xmm12, xmm12
		pxor xmm13, xmm13
		pxor xmm14, xmm14
		pxor xmm15, xmm15
		
		pxor mm0, mm0
		pxor mm1, mm1
		pxor mm2, mm2
		pxor mm3, mm3
		pxor mm4, mm4
		pxor mm5, mm5
		pxor mm6, mm6
		pxor mm7, mm7


		jmp _start




extern SystemSpaceBase
extern KeGlobalCR3

global KernelRelocate
extern __KernelRelocate
KernelRelocate:

	mov rax, InitData

	; Preserve address of InitData, _KernelStackTop
	mov r8, rax
	mov r10, __JumpToRelocatedKernel
	mov r9, [rel KeGlobalCR3]
	mov rax, [rax + 0x3C]
	push rax
	push r8
	push r9
	push r10
	call __KernelRelocate
	pop r10
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
	mov rbx, _KernelStackTop
	mov rbp, rbx ; Base Pointer = Stack Top

	mov rbx, __JumpToRelocatedKernel
	jmp rbx

global __JumpToRelocatedKernel


__JumpToRelocatedKernel:
	; Relocate Return Address
	
	pop rdx
	sub rdx, rax
	add rdx, rcx
	push rdx
	ret


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
	times 0x1000 resb 0 ; Protective Padding
	_KernelStackBottom:
	times 0x8000 resq 0
	_KernelStackTop:
	times 0x1000 resb 0 ; Protective Padding

