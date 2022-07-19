[BITS 64]

default rel

global __FastSysEntry

extern KeGlobalCR3
extern GlobalSyscallTable
extern Pmgrt

extern _syscall_max

PHYSICAL_STACK equ 0x10C
VIRTUAL_STACK_PTR equ 0x128
USER_STACK_BASE equ 0xFF00000000

__ENO_SYS equ 0x80000000 ; Invalid Syscall NR

section .text
[BITS 64]

default rel
__FastSysEntry:
	sti ; Enable interrupts (Automatically disabled on Sysenter)
	cmp rax, [rel _syscall_max]
	jnle .INVALID_SYSCALL_NUMBER

	; Prepare Environnement
	mov r14, cr3
	mov rdi, [rel KeGlobalCR3]
	mov cr3, rdi
	shl rax, 3 ; mul 8
	mov rbx, [rel GlobalSyscallTable]
	add rbx, rax

	call rbx
	mov cr3, r14
	mov rcx, r10 ; OS DEFINED SYSENTER RSP = R10
	mov rdx, r12 ; OS DEFINED SYSENTER RIP = R12
	o64 sysexit
.INVALID_SYSCALL_NUMBER:
    mov rax, __ENO_SYS
    o64 sysexit