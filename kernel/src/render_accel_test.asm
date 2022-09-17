[BITS 64]

section .text

global _SSE_ComputeBezier
global _AVX_ComputeBezier
; r8 = k, < RDX
; r11 = i, < R9
; R10 = RCX = Beta
; XMM1 = 1 - percent
; XMM2 = Percent
_SSE_ComputeBezier:
; for(register UINT k = 1;k < NumCordinates;k++)
    mov r8, 1
    mov r9, rdx
    cvtsi2ss xmm1, r8
    subss xmm1, xmm2
    movd eax, xmm1
    mov r11, rax
    shl r11, 32
    or rax, r11
    movq xmm1, rax

    movlhps xmm1, xmm1

    movd eax, xmm2
    mov r11, rax
    shl r11, 32
    or rax, r11
    movq xmm2, rax
    movlhps xmm2, xmm2

.loop0:
    cmp r8, rdx
    je .Exit
    ; for(register UINT i = 0;i<NumCordinates - k;i++)
    inc r8
    dec r9
    xor r11, r11
    mov r10, rcx
    .loop1:
        cmp r11, r9
        jae .loop0
        ; beta[i]:XMM0 = (1 - percent) * beta[i] + percent * beta[i + 1];

        ; XMM0 = (1 - percent) * beta[i]
        ; XMM0 = XMM1 * XMM3
        movaps xmm0, xmm1
        movups xmm3, [r10]
        mulps xmm0, xmm3
        ; XMM3 = percent * beta[i + 1]
        ; XMM3 = XMM2 * XMM4
        movaps xmm3, xmm2
        movups xmm4, [r10 + 4]
        mulps xmm3, xmm4
        ; XMM0 = XMM0 + XMM3
        addps xmm0, xmm3
        movups [r10], xmm0
        add r10, 0x10
        add r11, 4
        jmp .loop1
.Exit:
    movups xmm0, [rcx]
    cvtss2si rax, xmm0
    ret


; r8 = k, < RDX
; r11 = i, < R9
; R10 = RCX = Beta
; XMM1 = 1 - percent
; XMM2 = Percent
_SSE_ComputeBezierW:
; for(register UINT k = 1;k < NumCordinates;k++)
    mov r8, 1
    mov r9, rdx
    cvtsi2sd xmm1, r8
    subsd xmm1, xmm2

    movlhps xmm1, xmm1
    movlhps xmm2, xmm2
.loop0:
    cmp r8, rdx
    je .Exit
    ; for(register UINT i = 0;i<NumCordinates - k;i++)
    inc r8
    dec r9
    xor r11, r11
    mov r10, rcx 
    .loop1:
        cmp r11, r9
        jae .loop0
        ; beta[i]:XMM0 = (1 - percent) * beta[i] + percent * beta[i + 1];

        ; XMM0 = (1 - percent) * beta[i]
        ; XMM0 = XMM1 * XMM3
        movapd xmm0, xmm1
        movupd xmm3, [r10]
        mulpd xmm0, xmm3
        ; XMM3 = percent * beta[i + 1]
        ; XMM3 = XMM2 * XMM4
        movapd xmm3, xmm2
        movupd xmm4, [r10 + 0x08]
        mulpd xmm3, xmm4
        ; XMM0 = XMM0 + XMM3
        addpd xmm0, xmm3
        movupd [r10], xmm0
        add r10, 0x10
        add r11, 2
        jmp .loop1
.Exit:
    movaps xmm0, [rcx]
    cvtsd2si rax, xmm0
    ret

; r8 = k, < RDX
; r11 = i, < R9
; R10 = RCX = Beta
; YMM1 = 1 - percent
; YMM2 = Percent

; (float* beta:RCX, UINT NumCordinates:EDX, float percent:XMM2);
; Computing definitions are present in _SSE_ComputeBezier
_AVX_ComputeBezier:
    mov r8, 1
    mov r9, rdx
    
    cvtsi2ss xmm1, r8
    subss xmm1, xmm2
    sub rsp, 8
    movq [rsp], xmm1
    vbroadcastss ymm1, [rsp]
    sub rsp, 8
    movups [rsp], xmm2
    vbroadcastss ymm2, [rsp]
    add rsp, 0x10 ; Pop stack
.loop0:
    cmp r8, rdx
    je .Exit
    inc r8
    dec r9
    xor r11, r11
    mov r10, rcx
    .loop1:
        cmp r11, r9
        jae .loop0
        vmovaps ymm0, ymm1
        vmovups ymm3, [r10]
        vmulps ymm0, ymm3
        vmovaps ymm3, ymm2
        vmovups ymm4, [r10 + 4]
        vmulps ymm3, ymm4
        vaddps ymm0, ymm3
        vmovups [r10], ymm0
        add r10, 0x20
        add r11, 8
        jmp .loop1
.Exit:
    movups xmm0, [rcx]
    cvtss2si rax, xmm0
    ret