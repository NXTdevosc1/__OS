[BITS 64]

global EnterSystem
global __stdcall__EnterSystem

global __SyscallNumber



_USER_SYSTEM_CONFIG_ADDRESS equ 0x100000

SYSTEM_ENTER_PREFERRED_METHOD equ _USER_SYSTEM_CONFIG_ADDRESS ; 0 = SYSENTER, 8 = SYSCALL

section .text

__SyscallNumber:
    mov rax, rcx
    ret


__stdcall__EnterSystem:
EnterSystem:
    mov r12, [SYSTEM_ENTER_PREFERRED_METHOD]
    mov r10, __PreferredSysenterMethodPtr
    jmp [r10 + r12]

; IN THE OPERATING SYSTEM, SYSENTER HANDLER is FASTER Than SYSCALL HANDLER
; + SYSCALL instruction is slower than sysenter instruction

__Sysenter:
    pop r12 ; pop return address to r12 for direct return
    mov r10, rsp
    o64 sysenter

__Syscall:
    push rcx
    o64 syscall
    ret

__PreferredSysenterMethodPtr:
    dq __Sysenter
    dq __Syscall