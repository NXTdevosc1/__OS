
[BITS 64]
global SMP_TRAMPOLINE
global SMP_TRAMPOLINE_END

KERNEL_CS equ 0x08
KERNEL_DS equ 0x10


SMP_BOOT_ADDR equ 0x8000

GLOBAL_IDTR equ 0x8000
GLOBAL_GDTR equ 0x8008

; External Functions

KERNEL_PAGE_TABLE_POINTER equ 0x120000 ; As long as SMP are booting:
                                       ; This address contains the pointer to the page table
                                       ; After that it is cleared

SMP_INITIALIZATION_DATA equ SMP_TRAMPOLINE_END - SMP_TRAMPOLINE + 0x8000

ADDR_SMP_IDTR equ 0x8000 + (SMP_IDTR - SMP_TRAMPOLINE)
ADDR_SMP_GDTR equ 0x8000 + (SMP_GDTR - SMP_TRAMPOLINE)

ADDR_SMP_ENTRY equ 0x8000 + (SmpEntry - SMP_TRAMPOLINE)
[BITS 16]

section .text
align 0x1000

SMP_TRAMPOLINE:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov gs, ax
    mov fs, ax

    mov esp, 0xE800
    mov ebp, esp

    mov eax, (1 << 5) | (1 << 7) ; Physical Addess Extension & Page Global Enable
    mov cr4, eax

    xor eax, eax
    mov cr0, eax

    ; Set Page Table
    mov eax, PT_BASE
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    mov ecx, 0xC0000080

    or eax, 1 << 8 ; Set long mode enable
    wrmsr
    
    mov eax, 1 | (1 << 31) ; Protected Mode Enable & Set Paging Enable Bit
    mov cr0, eax



    xor eax, eax
    push eax
    popf ; Clear EFLAGS

    lidt [ADDR_SMP_IDTR]
    lgdt [ADDR_SMP_GDTR]
    
    jmp KERNEL_CS:ADDR_SMP_ENTRY
    hlt

align 0x10
SMP_IDTR:
    dw 0
    dd 0
align 0x10

SMP_GDTR:
    .Limit dw TMP_GDT_END - TMP_GDT - 1
    .Offset dq 0x8000 + (TMP_GDT - SMP_TRAMPOLINE)
align 0x10

TMP_GDT:
    .NULL dq 0
    .Code:
        ;dw 0xffff
        dw 0
        dw 0
        db 0
        db 10011010b
        ;db 11111010b
        db 1010b << 4 ; Limit | Flags << 4
        db 0
    .Data:
        ;dw 0xffff
        dw 0
        dw 0
        db 0
        db 10010010b
        ;db 11111000b
        db 1000b << 4 ; Limit | Flags << 4
        db 0

align 0x10
TMP_GDT_END:  
    

[BITS 64]
extern AllocatePool
extern SetupCPU
align 0x10

SmpEntry:
    mov ax, KERNEL_DS
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov gs, ax
    mov fs, ax
   
    mov rsp, 0xE800
    mov rbp, rsp

    ; Setup Initial CPU Requirements
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



    mov rax, [KERNEL_PAGE_TABLE_POINTER]
    mov cr3, rax

    mov rcx, 0x40000 ; 40K Stack Memory
    mov rbx, AllocatePool
    call rbx
    cmp rax, 0
    je .ResetSystem
    ; No Allocation Check

    add rax, 0x3F800 ; Go to stack top and leave 2k of free stack as padding
    mov rsp, rax
    mov rbp, rax
   
    ; --------------------------------

    

    mov rbx, SetupCPU
    call rbx

    .halt:
    pause
    hlt
    jmp .halt

    .ResetSystem:
        push 0
        push 0
        retfq
    
align 0x1000
PageTable:
PT_BASE equ 0x8000 + (PageTable - SMP_TRAMPOLINE)
; map to a 2MB Address space
.PML4:
    dq 0x3 + PT_BASE + 0x1000
    times 511 dq 0
.PDP:
    dq 0x3 + PT_BASE + 0x2000
    times 511 dq 0
.PD:
    dq 0x83 ; 2 MB Page
    times 511 dq 0

align 0x1000
SMP_TRAMPOLINE_END:
