[BITS 64]

section .text
global _AVX_MemsetUnaligned

global _SSE_MemcpyUnaligned

global memset
global memcpy
global memcmp

; RCX = Dest, DL = Value, R8 = Count
memset:
    mov al, dl
    mov rdi, rcx
    mov rcx, r8
    rep stosb
    ret
; RCX = Dest, RDX = Src, R8 = Count
memcpy:
    mov rsi, rdx
    mov rdi, rcx
    mov rcx, r8
    rep movsb
    ret
; RCX = buffer1, RDX = buffer2, R8 = Count
memcmp:
    mov rsi, rcx
    mov rdi, rdx
    mov rcx, r8
    repe cmpsb
    jne .failed
    mov rax, 1
    ret
.failed:
    xor rax, rax
    ret

; RCX = Address, RDX = Value, R8 = Count (In Bytes)
_AVX_MemsetUnaligned:
    push rdx
    vbroadcastsd ymm0, [rsp] ; Set Four YMM Locations to RDX (as a double precision floating point value)
    add rsp, 8
    shr r8, 5 ; Divide by 32
.loop:
    test r8, r8
    jz .Exit
    vmovdqu [rcx], ymm0
    add rcx, 0x20
    dec r8
    jmp .loop
.Exit:
    ret

global _SSE_MemsetUnaligned

_SSE_MemsetUnaligned:
    movq xmm0, rdx
    movlhps xmm0, xmm0
    shr r8, 4 ; Divide by 16
.loop:
    test r8, r8
    jz .Exit
    movdqu [rcx], xmm0
    add rcx, 0x10
    dec r8
    jmp .loop
.Exit:
    ret

; RCX = Dest, RDX = SRC, R8 = Count
_SSE_MemcpyUnaligned:
    shr r8, 4 ; Divide by 16
.loop:
    test r8, r8
    ; jz .
