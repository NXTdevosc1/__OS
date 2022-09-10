[BITS 64]

section .text

global _SSE_ComputeBezier

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