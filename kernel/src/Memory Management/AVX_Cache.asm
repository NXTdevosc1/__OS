[BITS 64]

section .text

global _AVX_AllocateMemorySegmentFromCache

MEMBLOCK_CACHE_SIZE equ 0x40

; RCX = CacheLine
_AVX_AllocateMemorySegmentFromCache:
    mov r8, rcx
    add rcx, 0x10 ; Offset to cache memory segments
    mov rdx, MEMBLOCK_CACHE_SIZE
.loop:
    test rdx, rdx
    jz .Exit
    vmovdqa ymm0, [rcx]
    movq rax, xmm0
    test rax, rax
    jz .E0
    lock bts qword [rcx], 0
    jc .E0
    mov qword [rcx], 0
    dec dword [r8]
    ret
.E0:
    add rcx, 8
    vpsrldq ymm0, ymm0, 8

.Exit:
    xor rax, rax
    ret