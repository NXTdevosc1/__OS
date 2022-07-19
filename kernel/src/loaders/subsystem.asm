global KernelThreadWrapper
global UserThreadWrapper


SYSCALL_POST_QUIT_MESSAGE equ 8
extern TerminateCurrentThread

[BITS 64]

section .text

align 0x1000
KernelThreadWrapper:
    call rax ; Entry function pointer
    mov rdi, rax
    call TerminateCurrentThread

align 0x1000
UserThreadWrapper:
    jmp $
    call rax ; Entry function pointer
    mov rdi, SYSCALL_POST_QUIT_MESSAGE
    mov rsi, rax ; exit code
    push 0 ; push null rcx
    o64 syscall
