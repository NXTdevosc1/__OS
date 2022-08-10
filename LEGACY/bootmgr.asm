; 64 BIT BOOT MANAGER
; READS KERNEL, GETS MEMORY MAP, SETUPS VESA VBE
; PERFORMS DIRECT JUMP TO LONG MODE




[BITS 16]
BOOT_PARTITION_BASE_ADDRESS equ 0x7C00

IMAGE_BASE equ 0x7E00
EXTENDED_BOOT_AREA_END equ 0x16C00
[ORG IMAGE_BASE]


BOOTMGR_SIZE equ 0x20000

MEMORY_BASE equ EXTENDED_BOOT_AREA_END

HARD_DRIVE_PAGE_COPY equ 0x500 ; Copy address for sectors



db "BOOTMGR_" ; magic 0 (8 bytes)
dw 0xFB7E ; magic 1 (2 bytes)
times 6 db 0; Padding

dw loader_start ; addr (2 bytes)

; BOOT_MANAGER_PARAMETER_TABLE
BootDrive db 0
dq 0 ; Padding
RealModeFunction dq 0 ; Address of function in real mode
RealModeReturn dq 0
KernelBufferAddress dq 0

AllocationTableLba dq 0
LastClusterChainSector dq 0xFFFFFFFFFFFFFFFF
DataClustersStartLba dq 0
Psf1FontAddress dq 0
dq 0
VirtualBufferSize dq 0
dq 0
ImageBase dq 0
KeImageNumSections dw 0
InitData dq 0
FileImportTable dq 0
KeSectionTable dq 0
KernelPeHdrAddress dq 0
; Boot Manager Routine Tables

DiskLbaPacket:
    .PacketSize db 0x10
    db 0
    .NumSectors dw 0
    .RealModeAddress dw DiskTransferBuffer
    .Keep0 dw 0 ; Transfer Buffer 0:DiskTransferBuffer
    .Lba dq 0
    ; 64 bit address of disk transfer buffer
    dq DiskTransferBuffer

align 0x200

VbeInfoBlock:
    .VbeSignature dd 'VBE2'
    .VbeVersion dw 0
    .OemStringPtr times 2 dw 0
    .Capabilities dd 0
    .VideoModePtr times 2 dw 0
    .TotalMemory dw 0

VbeReady db 0 ; To check if VBE is supported and enabled
VbeMode dw 0

align 0x200
VbeModeInfo:
    .attributes dw 0 ; Attributes
    .WindowA db 0
    .WindowB db 0
    .granularity dw 0
    .WindowSize dw 0
    .SegmentA dw 0
    .SegmentB dw 0
    .WinFunctionPtr dd 0
    .Pitch dw 0
    .Width dw 0
    .Height dw 0
    .Wchar db 0
    .Ychar db 0
    .Planes db 0
    .BitsPerPixel db 0
    .Banks db 0
    .MemoryModel db 0
    .BankSize db 0
    .ImagePages db 0
    .Reserved0 db 0
    .Redmask  db 0
    .RedPosition db 0
    .GreenMask db 0
    .GreenPosition db 0
    .BlueMask db 0
    .BluePosition db 0
    .ReservedMask db 0
    .ReservedPosition db 0
    .DirectColorAttributes db 0
    .FrameBufferAddress dd 0
    .OffsetScreenMemory dd 0
    .OffsetScreenMemorySize dw 0
    times 0x100 db 0 ; Reserved



align 0x10
DiskTransferBuffer:
    times 0x1000 db 0

stack_bottom:
    times 0x2200 db 0
stack_top:
times 0x100 db 0

ERROR_INVALID_FS_BOOTAREA db "Invalid or Corrupt File System Boot Area", 13, 10, 0
ERROR_UNSUPPORTED_FSBOOT_VERSION db "Unsupported File System Boot Version. BootManager Version : 1.0", 13, 10, 0
StrCPUError db "An unexpected CPU error has occured, halting...", 13, 10, 0

loader_start:
    cli

    mov [BootDrive], dl

    ; Disable NMI

    in al, 0x70
    and al, 0x7F
    out 0x70, al
    in al, 0x71

    ; Disable IRQ's
    mov al, 0xFF
    out 0xA1, al
    out 0x21, al



    ; SP Set by bootsector
    
    call EnableA20 ; Enables Fast A20 Gate



    call GetMemoryMap

    jmp EnableProtectedMode

.halt:
    hlt
    jmp .halt
; This is the best method availaible on all PC's since 2002 (and some previous ones)

BootLoaderMemory dq 0x40000 ; Used memory by bootloader (0x40000 As initialy used)

MemapCopy:
    test byte [BiosSystemMemoryMap + 20], 1 ; Present Bit ACPI
    jz .Exit ; Ignore
    push edi
    push eax

    ; Align region length
    mov esi, [BiosSystemMemoryMap + 8]
    and dword [BiosSystemMemoryMap + 8], ~(0xFFF)
    and esi, 0xFFF
    add [BootLoaderMemory], esi

    ; Check if length of region != 0
    mov esi, [BiosSystemMemoryMap + 8]
    test esi, esi
    jnz .SkipLengthCheck
    mov esi, [BiosSystemMemoryMap + 12]
    test esi, esi
    jz .SkipMap ; This is a clean entry

    .SkipLengthCheck:

    ; Check if the entry is inside system space (no need to check for kernel 0x8000-0x9000 cause they're approved in BIOS)
    mov esi, [BiosSystemMemoryMap]
    cmp esi, 0x40000
    ja .FinishSystemCheck ; Jmp if above otherwise check high 32 bits

    mov esi, [BiosSystemMemoryMap + 4]
    test esi, esi
    jz .UnmapSystemArea ; SYSTEM_AREA Consists of : 0x0 : IVT, 0x8000 : KERNEL SMP STARTUP
                        ; 0x9000 : KERNEL PML4

    .FinishSystemCheck:

    mov ax, [MemoryMap.Count]
    mov cl, SIZE_KERNEL_MEMORY_MAP
    mul cl
    mov di, MemoryMap.MemoryDescriptors
    add di, ax
    
    mov esi, [BiosSystemMemoryMap] ; Base Address Low 32 Bits
    mov [di + 9], esi
    mov esi, [BiosSystemMemoryMap + 4] ; Base Address High 32 Bits
    mov [di + 13], esi

    mov esi, [BiosSystemMemoryMap + 8] ; Aligned Page Count
    push ebx
    mov eax, esi
    shr esi, 12 ; Convert Byte Count to Page Count (0x1000 Bytes per Block)
    mov eax, [BiosSystemMemoryMap + 12]
    mov ebx, eax
    shl eax, 20
    or esi, eax
    mov eax, ebx
    pop ebx

    mov [di + 1], esi ; Page Count Low
    shr eax, 12 ; Remove last 12 bits (appended to low 32 bits of page count)
    mov [di + 5], eax ; Page Count High

    mov al, [BiosSystemMemoryMap + 16]
    mov [di], al

    inc word [MemoryMap.Count]
    .SkipMap:
    pop eax
    pop edi
.Exit:
    ret
.UnmapSystemArea:
    ; Check if length of region < 0x40000
    mov esi, [BiosSystemMemoryMap + 8]
    cmp esi, 0x40000
    ja .A000Valid ; BIOS Must map conventionnal memory in the <4GB area or all the memory will be lost
    jmp .SkipMap
    .A000Valid:

    mov dword [BiosSystemMemoryMap], 0x40000 ; 0-0x40000 Reserved for System
    sub dword [BiosSystemMemoryMap + 8], 0x40000
    jmp .FinishSystemCheck
    
GetMemoryMap:
    ; First Call
    mov eax, 0xE820
    xor ebx, ebx ; Continuation Value
    mov edx, 0x534D4150 ; Magic Number
    mov ecx, 24
    mov di, BiosSystemMemoryMap
    ; Set ignore bit if ACPI Field is not supported by BIOS
    mov word [es:di + 20], 1

    int 0x15


    jc .error
    cmp eax, 0x534D4150
    jne .error

    ; Copy memory map
    call MemapCopy

    .loop:
        test ebx, ebx ; Set to 0 if we reach end of list
        jz .exit
        mov eax, 0xE820
        mov edx, 0x534D4150 ; Magic Number
        mov ecx, 24
        ; Set ignore bit if ACPI Field is not supported by BIOS
        mov di, BiosSystemMemoryMap
        mov word [es:di + 20], 1

        int 0x15

        jc .exit
        cmp eax, 0x534D4150
        jne .error
        
        call MemapCopy
        add di, 24
        jmp .loop
    
    .exit:
    ret
    .error:
        mov di, FailedToGetMemoryMap
        jmp SetFailureMessage

EnableA20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret



EnableProtectedMode:
    xor ax,ax
    push ax
    popf
    lidt [IDT_REGISTER]
    lgdt [GDT_REGISTER]
    mov eax, cr0
    and eax, ~((3 << 2) | (3 << 30))
    or eax, 3 ; Set Protected Mode enable | Monitor coprocessor
    mov cr0, eax
    
    jmp 0x08:ProtectedModeEntry

ReEnterProtectedMode:
    lidt [IDT_REGISTER]
    lgdt [GDT_REGISTER]
    mov eax, cr0
    and eax, ~((3 << 2) | (3 << 30))
    or eax, 3 ; Set Protected Mode enable | Monitor coprocessor
    mov cr0, eax
    jmp 0x08:SetupSegmentsAndRet


_print16:
    
    push ax
    push di
    mov ah, 0x0e
	.loop:
		cmp byte [di], 0
		je .exit
		mov al, [di]
		inc di

        int 0x10

		jmp .loop
	.exit:
        pop di
        pop ax
        cli
    .halt:
        hlt
        jmp .halt
align 0x10
GDT_REGISTER:
    dw GDT_END - GDT - 1
    dq GDT

align 0x10
IDT_REGISTER:
    dw IDT_END - IDT - 1
    dq IDT




align 0x10
IDT:
dq 0
dq 0
IDT_END:



align 0x10
GDT:
    dq 0 ; Null segment
    .KernelCode:
        dw 0xffff
        dw 0
        db 0
        db 10011010b
        db 11001111b
        db 0
    .KernelData:
        dw 0xffff
        dw 0
        db 0
        db 10010010b
        db 11001111b
        db 0
    .Protected16BitModeCodeSegment:
        dw 0xffff
        dw 0
        db 0
        db 10011010b
        db 00001111b
        db 0
    ; Copy from Kernel 64-BIT GDT : CPU/descriptor_tables.h
    .KernelCode64:
        dw 0
        dw 0
        db 0
        db 10011010b
        db 1010b << 4 ; Limit | Flags << 4
        db 0
    .KernelData64:
        dw 0
        dw 0
        db 0
        db 10010010b
        db 1000b << 4 ; Limit | Flags << 4
        db 0
GDT_END:


SetupVesaVBE:
    ; Get info block
 
    pusha

    mov eax, 0x4F00
    mov edi, VbeInfoBlock
    push ds
    push es
    push gs
    push fs

    int 0x10

    pop fs
    pop gs
    pop es
    pop ds

    cmp eax, 0x4F ; With Status = 0
    mov edi, VesaVbeNotSupported
    jne SetFailureMessage

    
    cmp dword [VbeInfoBlock.VbeSignature], 'VESA'
    jne SetFailureMessage
    xor ebx, ebx
    mov bx, [VbeInfoBlock.VideoModePtr]; Count for loop
    .loop:
        mov eax, 0x4F01
        mov cx, [bx]
        cmp cx, 0xFFFF
        je .exit
        mov word [VbeMode], cx
        mov edi, VbeModeInfo
        push ds
        push es
        push gs
        push fs

        int 0x10

        pop fs
        pop gs
        pop es
        pop ds
        add bx, 2
        cmp ax, 0x4F
        jne .loop

        mov al, [VbeModeInfo.attributes]
        and al, 0x90
        cmp al, 0x90
        jne .loop
        cmp byte [VbeModeInfo.BitsPerPixel], 0x20
        jne .loop
        cmp word [VbeModeInfo.Width], 800
        jb .loop
        cmp word [VbeModeInfo.Height], 600
        jb .loop
        ; Find first resolution with 32 BPP
        ; And over/equal 800x600
        mov byte [VbeReady], 1
        jmp .exit

    .exit:
        mov di, VesaVbeNotSupported
        cmp byte [VbeReady], 0
        je SetFailureMessage
        ; Set Mode
        mov eax, 0x4F02
        mov bx, [VbeMode]
        or bx, 0x4000 ; To use the frame buffer
        xor edi, edi
        push ds
        push es
        push gs
        push fs

        int 0x10

        pop fs
        pop gs
        pop es
        pop ds
        cmp ax, 0x4F
        jne VesaVbeNotSupported

    popa
    ret

; Di = Message
SetFailureMessage:
    call _print16
    .halt:
    cli
    hlt
    jmp .halt

Protected16BitEnterRealMode:
    mov eax, cr0
    and eax, ~1
    mov cr0, eax


    lidt [RealModeIdt]

    jmp 0:RealModeEntry



RealModeEntry:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov gs, ax
    mov fs, ax

    mov ax, [RealModeFunction]
    call ax
    jmp ReEnterProtectedMode


Int13:
    pusha
    mov si, DiskLbaPacket
    mov ah, 0x42
    mov dl, [BootDrive]

    int 0x13

    jc .err
    popa
    ret
.err:
    mov di, HardDiskReadFailed
    call _print16
    .halt:
    hlt
    jmp .halt

[BITS 32]

RealModeCallEaxSaved dd 0

%macro CallRealMode 2
    mov [RealModeCallEaxSaved], eax
    mov word [RealModeFunction], %1
    mov word [RealModeReturn], %2
    jmp 0x18:Protected16BitEnterRealMode
%endmacro

; Last call to return to protected mode
SetupSegmentsAndRet:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov gs, ax
    mov fs, ax
    xor eax, eax
    mov eax, [RealModeCallEaxSaved]
    jmp [RealModeReturn]



ProtectedModeEntry:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov gs, ax
    mov fs, ax

    mov esp, stack_top
    mov ebp, esp
   
    ; Check BOOT_POINTER_TABLE

    cmp dword [BootPointerTable], BOOT_POINTER_TABLE_MAGIC
    jne .InvalidFsArea
    cmp dword [BootPointerTable + 4], BOOT_POINTER_TABLE_STRMAGIC
    jne .InvalidFsArea
    cmp word [BootPointerTable + 8], BOOT_MAJOR
    jne .UnsupportedFSBootVersion
    cmp word [BootPointerTable + 10], BOOT_MINOR
    jne .UnsupportedFSBootVersion

    

    jmp EnterLongMode
    .halt:
    hlt
    jmp .halt
.InvalidFsArea:
    mov di, ERROR_INVALID_FS_BOOTAREA
    call _print
    jmp .halt
.UnsupportedFSBootVersion:
    mov di, ERROR_UNSUPPORTED_FSBOOT_VERSION
    call _print
    jmp .halt



NewLine db 13, 10, 0




.error:
    mov ebx, FailedToGetMemoryMap
    call _print
    .halt:
        hlt
        jmp .halt



; Page Table Entry Definition : CPU/paging_defs.h

;typedef struct _PAGE_TABLE_ENTRY {
;    UINT64 Present : 1;
;    UINT64 ReadWrite : 1;
;     UINT64 UserSupervisor : 1;
;     UINT64 PageLevelWriteThrough : 1;
;     UINT64 PageLevelCacheDisable : 1;
;     UINT64 Accessed : 1;
;     UINT64 Dirty : 1;
;     UINT64 Size : 1;
;     UINT64 Global : 1;
;     UINT64 Ignored0 : 3;
;     UINT64 PhysicalAddr : 36;
;     UINT64 Ignored1 : 15;
;     UINT64 ExecuteDisable : 1;
; } PTBLENTRY, *HPAGEMAP;
; -------------------

; ebx = address, ecx = NumPages
; FUNCTION_REGISTERS : {[eax] = PT, [edx] = PD, [edi] = PDP, [esi] = PML4 }
; EBP is used to address page tables, and not for stack :)
; Return VALUE : Bootloader Halts on Failure
; MapPages32: ; Map Phys-to-Phys Pages (no Virt-Phys translation)
; pushad
; .loop:
;     test ecx, ecx    
;     jz .Finish
; ; Calculate Indexes
;     ; PT INDEX
;     mov eax, ebx
;     shr eax, 12
;     and eax, 0x1FF
;     ; PD INDEX
;     mov edx, ebx
;     shr edx, 21
;     and edx, 0x1FF
;     ; PDP INDEX
;     mov edi, ebx
;     shr edi, 30
;     and edi, 0x1FF
;     ; PML4 INDEX
;     mov esi, ebx
;     shr esi, 39
;     and esi, 0x1FF
; ; Multiply indexes by 8
;     shl eax, 3
;     shl edx, 3
;     shl edi, 3
;     shl esi, 3
; ; PML4 Allocation
;     lea ebp, [PageTable + esi]
;     test dword [ebp], 1 ; Present
;     jz .AllocatePml4
;     .retpml4alloc:
; ; PDP Allocation
;     mov ebp, [ebp]
;     and ebp, ~(0xFFF)
;     lea ebp, [ebp + edi]
;     test dword [ebp], 1
;     jz .AllocatePdp
;     .retpdpalloc:
; ; PD Allocation
;     mov ebp, [ebp]
;     and ebp, ~(0xFFF)
;     lea ebp, [ebp + edx]
;     test dword [ebp], 1
;     jz .AllocatePd
;     .retpdalloc:
; ; PT Setup
;     mov ebp, [ebp]
;     and ebp, ~(0xFFF)
;     lea ebp, [ebp + eax]
;     mov eax, 3 ; Present Bit
;     or eax, ebx
;     mov [ebp], eax
; ; Continue Loop
;     add ebx, 0x1000
;     dec ecx
;     jmp .loop
; .Finish:
    
;     popad
;     ret
; .AllocatePml4:
;     call .AllocatePageTableEntry
;     jmp .retpml4alloc
; .AllocatePdp:
;     call .AllocatePageTableEntry
;     jmp .retpdpalloc
; .AllocatePd:
;     call .AllocatePageTableEntry
;     jmp .retpdalloc
; .AllocatePageTableEntry:
;     push eax
;     push ecx
;     mov ecx, 1
;     call AllocatePages32
;     test eax, eax
;     jz .err

;     ; Clear Memory
;     push eax
;     push edi
;     mov edi, eax
;     xor eax, eax
;     mov ecx, 0x400 ; 0x400 DWORDS
;     rep stosd
;     pop edi
;     pop eax

;     mov ecx, 3 ; Set present & read write bit
;     or ecx, eax
;     mov [ebp], ecx
;     pop ecx
;     pop eax
;     ret

; .err:
;     mov di, NoEnoughMemory
;     call _print
;     cli
;     jmp halt



halt:
    hlt
    jmp halt
; Allocates Constant Pages that cannot be freed up
; ecx = Num Pages
; Return value : eax = Address
; Primarely used for allocating page tables in 32 bits
ALLOC_PAGES_EAX dd 0
AllocatePages32:
    pushad
    mov dword [ALLOC_PAGES_EAX], 0
    mov eax, [MemoryMap.Count]
    mov ebx, MemoryMap.MemoryDescriptors
    ; Set edi to max address
    mov edi, ecx
    shl edi, 12
    mov esi, edi
    mov edi, 0xFFFFF000
    sub edi, esi
    ; Search For free memory
.loop:
    test eax, eax
    jz .Exit ; Return 0
    ; Check memory type
    mov dl, [ebx]
    cmp dl, 1
    jne .ContinueSearch ; Must be a conventional memory
    cmp dword [ebx + 13], 0
    jne .ContinueSearch ; Ram is above 4GB
    mov edx, [ebx + 9]
    cmp edx, edi
    jae .ContinueSearch ; Resulting ram access will be < 4GB
    ; Calculate page count as lower ram (for e.g. if page count = 0x4000000 it is considered 0)
    cmp [ebx + 1], ecx
    jb .ContinueSearch
    ; Availaible memory is found
    mov eax, [ebx + 9]; Address
    mov [ALLOC_PAGES_EAX], eax
    sub [ebx + 1], ecx
    shl ecx, 12
    add [ebx + 9], ecx
    jmp .Exit
.ContinueSearch:
    dec eax
    add ebx, SIZE_KERNEL_MEMORY_MAP
    
.Exit:

    popad
    mov eax, [ALLOC_PAGES_EAX]
    ret

HexBuffer times 0x12 db 0

; eax = number, buffer = HexBuffer

; TO_HEX Convertion :
; Assembly Copy from stdlib osdll.dll (Converted by hand by developper)



_print:
    CallRealMode _print16, .ext
    .ext:
    ret

align 0x10
RealModeIdt:
    dw 0x3FF
    dd 0 ; Real Mode Interrupt Vector Table


align 0x10
BiosSystemMemoryMap: times 24 db 0 ; SMAP From bios converted to memory map

MAX_MEMAP_ENTRIES equ 255
SIZE_KERNEL_MEMORY_MAP equ 17
align 0x10
MemoryMap:
    .Count dq 0
    ; .BootLoaderMemory dq 0 Constant memory used by bootloader
    .MemoryDescriptors times MAX_MEMAP_ENTRIES * SIZE_KERNEL_MEMORY_MAP db 0


FailedToGetMemoryMap db "An unexpected error ocurred. Failed to get Memory Map.", 13, 10, 0
bmgr db "Legacy Bios OS Boot Manager 1.0", 13, 10, 0
VesaVbeNotSupported db "VESA VBE Not Supported. This computer cannot run the Operating System.", 13, 10, 0

__SUCCESS db "SUCCESS...", 13, 10, 0
HardDiskReadFailed db "An unexpected error occurred, Failed to read from Hard Drive.", 13, 10, 0
ProcessorDoesNotSupport64Bit db "Processor Does not support 64 Bit Mode. This computer cannot run the Operating System.", 13, 10, 0
NoEnoughMemory db "Loading Failed, no enough memory.", 13, 10, 0







%include "bootldr64.asm"
times 0x12200 - ($-$$) db 0



; Basic 4MB Page table to enter long mode
PageTable:
.Pml4:
dq .Pdp + 3 ; + 3 (or | 3 ) which set Present & R/W Bits
times 511 dq 0
.Pdp:
dq .Pd + 3
times 511 dq 0
.Pd:
dq .Pt + 3
times 511 dq 0
.Pt:
%assign i 0
%rep 512
dq i + 3 ; Physical Address of Page | 3 (Present & R/W)
%assign i i + 0x1000
%endrep

align 0x10
ClusterChainSector times 0x200 db 0

BOOT_MAJOR equ 1
BOOT_MINOR equ 0


times BOOTMGR_SIZE - ($-$$) db 0

BootPointerTable: ; Setup in FS To eliminate FS & Disk drivers usage and acquire faster load performance 

%include "bptdefs.asm"