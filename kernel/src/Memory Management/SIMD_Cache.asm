[BITS 64]

section .text
MEMBLOCK_CACHE_SIZE equ 0x40

global _SSE_FetchMemoryCacheLine


; RCX = CacheLine
_SSE_FetchMemoryCacheLine:
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
    lock bts dword [rax], 0 ; Set present bit
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
    lock bts dword [rax], 0 ; Set present bit
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

global _AVX_FetchMemoryCacheLine


; RCX = CacheLine
_AVX_FetchMemoryCacheLine:
    hlt
    jmp _AVX_FetchMemoryCacheLine
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
    lock bts dword [rax], 0 ; Set present bit
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

global _AVX512_FetchMemoryCacheLine

; RCX = CacheLine
_AVX512_FetchMemoryCacheLine:
    hlt
    jmp _AVX512_FetchMemoryCacheLine