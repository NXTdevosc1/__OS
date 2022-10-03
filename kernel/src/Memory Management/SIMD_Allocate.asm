[BITS 64]

section .text

global _SSE_FetchSegmentListHead

MEMORY_LIST_HEAD_SIZE equ 0x40


; SIMD Optimized Single-Page Allocation (Essentially created for paging and such tasks)
global _SSE_AllocatePhysicalPage


%macro _APP0 0
    mov rax, 1
    movq r9, xmm0
    bsf r10, r9
    jz %%.EXIT
    lock btr qword [rcx], r10
    jnc %%.EXIT
    shl r10, 3
    add r8, r10
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
    ; mov rbx, [rel MemoryManagementTable] ; Available Memory
    ; cmp rbx, 0x1000
    ; jb .ExitFailure
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

