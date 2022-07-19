[BITS 64]

default rel

global _SyscallEntry
global _syscall_max
extern GlobalSyscallTable
extern Pmgrt

extern KeGlobalCR3
__ENO_SYS equ 0x80000000 ; Invalid Syscall NR
KERNEL_DS equ 0x10

USER_DS equ 0x23

CURRENT_THREAD equ 8 
PHYSICAL_STACK equ 0x10C
VIRTUAL_STACK_PTR equ 0x128
USER_STACK_BASE equ 0xFF00000000

; ALL SYSTEM CALLS ARE PERFORMED WITH MICROSOFT ABI __cdecl



section .text

_SyscallEntry:
    mov r10, rcx
    
    pop rcx
    cmp rax, [rel _syscall_max]
    jnle .INVALID_SYSCALL_NUMBER

    ; Prepare environnement
    mov r14, cr3

    mov r13, [rel Pmgrt + CURRENT_THREAD]
    
    sub rsp, [r13 + VIRTUAL_STACK_PTR]
    add rsp, [r13 + PHYSICAL_STACK]

    mov rdi, [rel KeGlobalCR3]
    mov cr3, rdi

    mov di, KERNEL_DS
    mov ds, di

    shl rax, 3 ; mul 8
    mov rbx, [rel GlobalSyscallTable]
    add rbx, rax

    push 0 ; Function padding
    call rbx
    pop rdi
    mov di, USER_DS
    mov ds, di

    mov cr3, r14
    sub rsp, [r13 + PHYSICAL_STACK]
    add rsp, [r13 + VIRTUAL_STACK_PTR]

    mov rcx, r10
    o64 sysret
.INVALID_SYSCALL_NUMBER:
    mov rax, __ENO_SYS
    o64 sysret



section .data
_syscall_max dq 1
times 0x1000 db 0
