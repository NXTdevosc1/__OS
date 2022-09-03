[BITS 64]

section .text

global _SSE_BezierCopyCords
global _SSE_ComputeBezier
; RCX = DEST (Reserved), RDX = SRC, R8b = NumCords
_SSE_BezierCopyCords:
    test r8b, 1
    jz .loop
    inc r8b
; Fill XMM8-XMM15 with betas
.loop:
    test r8b, r8b
    jz .Exit
    movdqu xmm8, [rdx]
    sub r8b, 2
    add rcx, 0x10
    add rdx, 0x10
    test r8b, r8b
    jz .Exit
    movdqu xmm9, [rdx]
    sub r8b, 2
    add rcx, 0x10
    add rdx, 0x10
    test r8b, r8b
    jz .Exit
    movdqu xmm10, [rdx]
    sub r8b, 2
    add rcx, 0x10
    add rdx, 0x10
    test r8b, r8b
    jz .Exit
    movdqu xmm11, [rdx]
    sub r8b, 2
    add rcx, 0x10
    add rdx, 0x10
    test r8b, r8b
    jz .Exit
    movdqu xmm12, [rdx]
    sub r8b, 2
    add rcx, 0x10
    add rdx, 0x10
    test r8b, r8b
    jz .Exit
    movdqu xmm13, [rdx]
    sub r8b, 2
    add rcx, 0x10
    add rdx, 0x10
    test r8b, r8b
    jz .Exit
    movdqu xmm14, [rdx]
    sub r8b, 2
    add rcx, 0x10
    add rdx, 0x10
    test r8b, r8b
    jz .Exit
    movdqu xmm15, [rdx]
.Exit:
    ret

%macro MACRO__SSE_CalculateBetaFromXMM 2
    movq rdi, 1
    movq xmm3, rdi
    movlhps xmm3, xmm3 ; Use parallel computing
    subpd xmm3, xmm2 ; (1 - percent)
    mulpd xmm3, xmm%1 ; * beta[i]
    movaps xmm4, xmm2
    ; percent * beta[i + 1];
    ; Pack multipliers into XMM5
    movaps xmm5, xmm%1
    movlhps xmm%2, xmm6
    ; Multiply the two values
    mulpd xmm4, xmm5
    ; Add the two groups
    addpd xmm3, xmm4
    ; beta[i] = (1 - percent) * beta[i] + percent * beta[i + 1];
    movaps xmm%1, xmm3


%endmacro

%assign i 8
%rep 7

%assign i i + 1
%endrep

CalculateBetaFromXMM_Ptrs:

; RCX = Beta (Reserved), edx = NumCordinates, XMM2 = percent (t)
; R10b = K, R11b = i, R13b = IMAX

; for(register UINT k = 1;k < NumCordinates;k++) {
; 		for(register UINT i = 0;i<NumCordinates - k;i++) {
; 			beta[i] = (1 - percent) * beta[i] + percent * beta[i + 1];
; 		}
; 	}


_SSE_ComputeBezier:
    mov r10b, 1
    mov r13b, dl
    movlhps xmm2, xmm2
.loop0:
    cmp r10b, dl
    je .Exit
    xor r11b, r11b
    .loop1:
        cmp r11b, r13b
        je .Loop1Exit
        
        ; beta[i] = (1 - percent) * beta[i] + percent * beta[i + 1];
    
    
        inc r11b
        jmp .loop1
    .Loop1Exit:
        inc r10b
        dec r13b
        jmp .loop0
.Exit:
    movaps xmm0, xmm8 ; Beta [0]
    ret