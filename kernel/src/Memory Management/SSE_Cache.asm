[BITS 64]

section .text

global _SSE_AllocateMemorySegmentFromCache

MEMBLOCK_CACHE_SIZE equ 0x40

; RCX = CacheLine
_SSE_AllocateMemorySegmentFromCache:
    mov r8, rcx
    add rcx, 0x10 ; Offset to cache memory segments
    mov rdx, MEMBLOCK_CACHE_SIZE
.loop:
    test rdx, rdx
    jz .Exit
    movdqa xmm0, [rcx]
    movq rax, xmm0
    test rax, rax
    jz .E0
    lock bts qword [rcx], 0 ; Set bit to check
    jc .E0 ; Taken by another CPU
    mov qword [rcx], 0
    dec dword [r8] ; Cache Line Size
    ret
.E0:
    add rcx, 8
    psrldq xmm0, 8
    movq rax, xmm0
    test rax, rax
    jz .E1
    lock bts qword [rcx], 0
    jc .E1
    mov qword [rcx], 0
    dec dword [r8]
    ret
.E1:
    add rcx, 8
    sub rdx, 2
    jmp .loop
.Exit:
    xor rax, rax
    ret