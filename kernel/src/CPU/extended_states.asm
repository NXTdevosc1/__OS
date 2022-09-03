global EnableExtendedStates
cpu_table_ptr equ 0x2300

section .text

FPUCTL_37F dq 0x37F
FPUCTL_37E dq 0x37E
FPUCTL_37A dq 0x37A

_MXCSR_MASK dq 0x1F80

EnableExtendedStates:
    push rax
    push rbx
    push rcx
    push rdx

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

    FNINIT
    push 0
    FNSTSW [rsp]
    pop rax
    test rax, rax
    jnz .NoFpu

;     ; Load MXCSR
;     sub rsp, 0x200
;     fxsave [rsp]
;     mov eax, [rsp + 28] ; MXCSR_MASK
;     or eax, eax
;     jnz ._A
;     mov eax, 0xFFBF
; ._A:
;     sub rsp, 8
;     and eax, 0x1F80
;     mov [rsp], rax
;     ; ldmxcsr [rsp]

;     add rsp, 0x208

    mov eax, 1
    cpuid
    ; XSAVE
    test ecx, 1 << 26
    jz .A0
    push rax
    mov rax, cr4
    or rax, 1 << 18
    mov cr4, rax
    pop rax
.A0:


    ; AVX
    test ecx, 1 << 28
    jz .A1
    push rax
    push rcx
    push rdx

    xor rcx, rcx
    xgetbv
    or eax, 7 ; X87 FPU, SSE, AVX
    xsetbv

    pop rdx
    pop rcx
    pop rax
.A1:

    ; AVX 512
    mov eax, 0xD
    cpuid
    mov rbx, rax
    and rbx, 7 << 5
    cmp rbx, 7 << 5
    jne .A2
    push rax
    push rcx
    push rdx
    xgetbv
    or eax, 1 << 6 ; AVX-512 & lower ZMM Enabled
    pop rdx
    xsetbv
    pop rcx
    pop rax

.A2:

    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret
.NoFpu:
    cli
    mov rax, 0xbadcafe
    hlt
    jmp .NoFpu
