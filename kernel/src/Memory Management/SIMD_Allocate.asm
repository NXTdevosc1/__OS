[BITS 64]

section .text

global _SSE_FetchSegmentListHead

MEMORY_LIST_HEAD_SIZE equ 0x40


; SIMD Optimized Single-Page Allocation (Essentially created for paging and such tasks)
global _SSE_AllocatePhysicalPage


%macro _APP0 0
    movq r9, xmm0
    not r9
    bsf r10, r9
    jz %%.EXIT
    bts qword [rcx], r10
    mov [r11 + 0x60], r8
    shl r10, 3
    add r8, r10
    or qword [r8], 1
    mov rax, [r8]
    and rax, ~0xFFF

    
    mov [r11 + 0x58], rcx
    ret
%%.EXIT:
    add r8, 64 * 8
    psrldq xmm0, 8
    add rcx, 0x08
%endmacro

; (char* PageBitmap, UINT64 BitmapSize, PAGE* PageArray)

extern MemoryManagementTable

_SSE_AllocatePhysicalPage:

    mov r11, MemoryManagementTable
    mov rcx, [r11 + 0x58] ; FreePagesStart
    mov r8, [r11 + 0x60] ; FreePageArrayStart
    mov rdx, [r11+ 0x68] ; BmpEnd
.loop0:
    movdqu xmm0, [rcx] ; Page Bitmap

    %rep 2
    _APP0
    %endrep
    cmp rcx, rdx
    jae .ExitFailure

    jmp .loop0
.loop1:
    movdqu xmm0, [rcx] ; Page Bitmap

    %rep 2
    _APP0
    %endrep
    cmp rcx, rdx
    jae .ExitFailure1

    jmp .loop1
.ExitFailure:
    ; Maybe Free pages start overlapped, search the entire page bitmap
    mov rcx, [r11 + 0x50] ; PageBitmap
    mov r8, [r11 + 0x40] ; Page Array
    jmp .loop1
.ExitFailure1:
    xor rax, rax
    ret

