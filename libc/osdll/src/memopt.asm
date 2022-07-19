[BITS 64]
global _Xmemset128
export _Xmemset128

global _Xmemset256
export _Xmemset256

global _Xmemset512
export _Xmemset512

section .text


_Xmemset128:
    push rdx
    push rdx
    movdqu xmm0, [rsp]
    pop rdx
    pop rdx
    .loop:
        test r8, r8
        jz .exit
        movdqu [rcx], xmm0
        add rcx, 16
        dec r8
        jmp .loop
    .exit:
        ret
_Xmemset256:
    ret
_Xmemset512:
    ret

global __memset64
export __memset64

global __memset32
export __memset32

global __memset16
export __memset16

global __memset
export __memset

; __memset(dest, value, size)
__memset64:
    cld
    mov rdi, rcx ; Address
    mov rcx, r8 ; Repetitions
    mov rax, rdx ; Value
    rep stosq
    ret

__memset32:
    cld
    mov rdi, rcx ; Address
    mov rcx, r8 ; Repetitions
    mov eax, edx ; Value
    rep stosd
    ret

__memset16:
    cld
    mov rdi, rcx ; Address
    mov rcx, r8 ; Repetitions
    mov ax, dx ; Value
    rep stosw
    ret

__memset:
    cld
    mov rdi, rcx ; Address
    mov rcx, r8 ; Repetitions
    mov ax, dx ; Value
    rep stosb
    ret