[BITS 64]

section .text

global _SSE_FetchSegmentListHead

MEMORY_LIST_HEAD_SIZE equ 0x40


; SIMD Optimized Single-Page Allocation (Essentially created for paging and such tasks)
global _SSE_AllocatePhysicalPage


%macro _APP0 0
    mov rax, 1
    movq rdi, xmm0
    bsf rsi, rdi
    jz %%.EXIT
    lock btr qword [rcx], rsi
    jnc %%.EXIT
    shl rsi, 3
    add r8, rsi
    or qword [r8], 1
    mov rax, [r8]
    and rax, ~0xFFF
    ret
%%.EXIT:
    add r8, 64 * 8
    psrldq xmm0, 8
    add rcx, 0x08
%endmacro

; (char* PageBitmap, UINT64 BitmapSize, PAGE* PageArray)

extern MemoryManagementTable

_SSE_AllocatePhysicalPage:
    mov rbx, [rel MemoryManagementTable] ; Available Memory
    cmp rbx, 0x1000
    jb .ExitFailure
.loop0:
    movdqa xmm0, [rcx] ; Page Bitmap
    %rep 2
    _APP0
    %endrep
    sub rdx, 0x10
    jmp .loop0
.ExitFailure:
    xor rax, rax
    ret


global _SSE_AllocateContiguousPages

%macro _SSE_ACP_LOOP1 1 ; C Offset
    test rdi, rax
    jz %%.ON
    ; Release chain of pages
    xor r10, r10
    jmp %%.EXIT
%%.ON:
    test r10, r10
    jnz %%.A
    ; Set RBX, RSI, R10
    mov rbx, rdx
    mov sil, %1
%%.A:
    inc r10
    cmp r10, rcx
    je .ExitSuccess
%%.EXIT:
%endmacro

; RCX = NUM_PAGES, RDX = Page Bitmap, R8 = Page Bitmap Size in Bytes
; 128 Times LOOP on XMM0 [128-BIT]
; RBX = Start Address
; RSI = Bit Offset
; R10 = Allocated pages
_SSE_AllocateContiguousPages:
.loop0:
    movdqa xmm0, [rdx]
    xor r10, r10 ; Current Pages
    .loop1:
        %rep 2
            %assign i 0
            mov rax, 1
            movq rdi, xmm0
            %rep 64
            _SSE_ACP_LOOP1 i
            %assign i i + 1
            shl rax, 1
            %endrep
            psrldq xmm0, 8
            add rdx, 0x8
        %endrep
    .loop1_exit:
    
    dec r8
    jmp .loop0

.ExitSuccess:
    mov rax, rbx ; Start Address
    mov rbx, 0xccc
    jmp $
    ; Fill the chain of bits
.Exit: ; Failure
    xor rax, rax
    ret