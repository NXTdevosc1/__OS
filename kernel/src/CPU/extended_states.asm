global EnableExtendedStates
cpu_table_ptr equ 0x2300

section .text

EnableExtendedStates:
    push rax
    push rbx
    push rcx
    push rdx


    ; Enable SSE
    mov rax, cr0
    and eax, ~(4) ; Clear Emulation
    or eax, 2
    mov cr0, rax
    mov rax, cr4
    or rax, (3 << 9) ; OFXSR | OSXMMEXCPT
    mov cr4, rax

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