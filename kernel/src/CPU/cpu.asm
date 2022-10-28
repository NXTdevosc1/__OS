[bits 64]

global LoadGDT
global FlushTSS
global _Xmemset128
global _Xmemset256
global _Xmemset512

global _basicX64Init

global _TimeSliceCounter

global _getCr4
global _setCr4
global _xgetbv
global _xsetbv

GDT_KERNEL_CODE_SEGMENT equ 0x08
GDT_KERNEL_DATA_SEGMENT equ 0x10



extern _start

section .text

LoadGDT:
	lgdt [rcx]
	mov ax, GDT_KERNEL_DATA_SEGMENT
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	pop rax
	push qword GDT_KERNEL_CODE_SEGMENT
	push rax
    retfq


; null off = 0, kernelc = 0x08, kerneld = 0x10, userc = 0x18, userd = 0x20, tss = 0x28

FlushTSS:
	ltr cx
	ret



; _Xmemset128:
;     movq xmm0, rdx
;     movlhps xmm0, xmm0
;     .loop:
;         test r8, r8
;         jz .exit
;         movdqu [rcx], xmm0
;         add rcx, 16
;         dec r8
;         jmp .loop
;     .exit:
;         ret
; _Xmemset256:
;     movq xmm0, rsi
;     movlhps xmm0, xmm0
;     .loop:
;         cmp rdx, 0
;         je .exit
;         vmovdqa [rdi], ymm0
;         add rdi, 32
;         dec rdx
;         jmp .loop
;     .exit:
;     ret
; _Xmemset512:
;     push rsi
;     push rsi
;     push rsi
;     push rsi
;     push rsi
;     push rsi
;     push rsi
;     push rsi
;     vmovaps zmm0, [rsp]
;     pop rsi
;     pop rsi
;     pop rsi
;     pop rsi
;     pop rsi
;     pop rsi
;     pop rsi
;     pop rsi
;     cli
;     hlt
;     .loop:
;         cmp rdx, 0
;         je .exit
;         vmovaps [rdi], zmm0
;         add rdi, 64
;         dec rdx
;         jmp .loop
;     .exit:
;     ret

_basicX64Init:
     ; enable write protect
    mov rax, cr0
    or rax, 1 << 16 ; WP
    mov cr0, rax

    ; Enable SSE
    mov rax, cr0
    and rax, ~(3 << 2) ; Clear Emulation & Task Switched
    or rax, 2
    mov cr0, rax
    mov rax, cr4
    or rax, (3 << 9) ; OFXSR | OSXMMEXCPT
    mov cr4, rax

    fninit

    ret

_getCr4:
    mov rax, cr4
    ret
_setCr4:
    mov cr4, rcx
    ret
_xgetbv:
    ; ECX = XSTATE
    xgetbv
    ret
_xsetbv:
    ; ECX = XSTATE
    mov eax, edx
    xsetbv
    ret