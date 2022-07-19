[BITS 64]

; PAGE ENTRY MASK
	PE_PRESENT equ 1
	PE_READ_WRITE equ 2
	PE_USER_SUPERVISOR equ 4
	PE_ADDR_SHIFT equ 12
	PE_ADDR_MASK equ (0xFFFFFFFFF << PE_ADDR_SHIFT)

PE_SIZE equ 8
section .text

global __ASM_MapPhysicalPages
extern kpalloc
extern _Xmemset128




; r15 : PtIndex
; r9  : PdIndex
; r10 : PdpIndex
; r11 : Pml4Index
; r12 : Counter
; rsi : Virtual Address
; rdx : Physical Address
; r8  : Flags
; rdi : Parameters
; rbx : Processing Register

; MapPhysicalPages([rdi] HPAGEMAP PageMap,
;    [rsi] LPVOID VirtualAddress,
;    [rdx] LPVOID PhysicalAddress,
;    [rcx] UINT64 Count,
;    [r8] UINT64 Flags)

section .text

__ASM_MapPhysicalPages:

	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	push rbx
	push rdx
	push rsi
	push rdi

	; get page indexs
	shr rsi, 12 
	shr rdx, 12
	xor r12, r12
	mov r13, rdi
	.MapLoop:
		cmp r12, rcx
		je .exit
		.IndexEntries:
		mov r15, rsi
		and r15, 0x1FF

		mov r9, rsi
		shr r9, 9
		and r9, 0x1FF

		mov r10, rsi
		shr r10, 18
		and r10, 0x1FF

		mov r11, rsi
		shr r11, 27
		and r11, 0x1FF

		mov r14, r13 ; PageMapPtr
		imul r11, PE_SIZE
		add r14, r11
		.pml4:
			test qword [r14], PE_PRESENT
			jnz .pml4_set
			call .Allocate
			xor rbx, rbx
			or rbx, PE_PRESENT | PE_READ_WRITE | PE_USER_SUPERVISOR
			or rbx, rax ; PageMapAddress
			mov [r14], rbx
			mov r14, rax
			call .ClearPageMap
			jmp .pdp
			.pml4_set:
				mov r14, [r14]
				mov rdi, PE_ADDR_MASK
				and r14, rdi
		.pdp:
			imul r10, PE_SIZE
			add r14, r10
			test qword [r14], PE_PRESENT
			jnz .pdp_set
			call .Allocate
			xor rbx, rbx
			or rbx, PE_PRESENT | PE_READ_WRITE | PE_USER_SUPERVISOR
			or rbx, rax
			mov [r14], rbx
			mov r14, rax
			call .ClearPageMap
			jmp .pd
			.pdp_set:
				mov r14, [r14]
				mov rdi, PE_ADDR_MASK
				and r14, rdi

		.pd:
			imul r9, PE_SIZE
			add r14, r9
			test qword [r14], PE_PRESENT
			jnz .pd_set
			call .Allocate
			xor rbx, rbx
			or rbx, PE_PRESENT | PE_READ_WRITE | PE_USER_SUPERVISOR
			or rbx, rax
			mov [r14], rbx
			mov r14, rax
			call .ClearPageMap
			jmp .pt
			.pd_set:
				mov r14, [r14]
				mov rdi, PE_ADDR_MASK
				and r14, rdi
		
		.pt:
			imul r15, PE_SIZE
			add r14, r15
			xor rbx, rbx
			or rbx, PE_PRESENT | PE_READ_WRITE | PE_USER_SUPERVISOR
			shl rdx, PE_ADDR_SHIFT
			or rbx, rdx
			shr rdx, PE_ADDR_SHIFT
			mov [r14], rbx

		inc r12
		inc rsi
		inc rdx
		jmp .MapLoop
	.exit:
		pop rdi
		pop rsi
		pop rdx
		pop rbx
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		xor rax, rax
		ret
	.ClearPageMap:
		push rdx
		push rsi

		
		mov rdi, rax
		xor rsi, rsi
		mov rdx, 256
		call _Xmemset128
		pop rsi
		pop rdx
		ret
	.Allocate:
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15
		push rdx
		push rsi
		push rcx
		push r8

		mov rdi, 1
		call kpalloc
		pop r8
		pop rcx
		pop rsi
		pop rdx
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		ret