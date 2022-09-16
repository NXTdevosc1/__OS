; 64 BIT BOOT MANAGER
; READS KERNEL, GETS MEMORY MAP, SETUPS VESA VBE
; PERFORMS DIRECT JUMP TO LONG MODE




[BITS 16]
BOOT_PARTITION_BASE_ADDRESS equ 0x1FE00

IMAGE_BASE equ 0x20000
EXTENDED_BOOT_AREA_END equ 0x16C00
[ORG 0x200] ; Offset from bootsector


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
__KernelBufferAddress dq 0
KernelBufferAddress equ __KernelBufferAddress + BOOT_PARTITION_BASE_ADDRESS
__AllocationTableLba dq 0
AllocationTableLba equ __AllocationTableLba + BOOT_PARTITION_BASE_ADDRESS
__LastClusterChainSector dq 0xFFFFFFFFFFFFFFFF
LastClusterChainSector equ __LastClusterChainSector + BOOT_PARTITION_BASE_ADDRESS
__DataClustersStartLba dq 0
DataClustersStartLba equ __DataClustersStartLba + BOOT_PARTITION_BASE_ADDRESS
__Psf1FontAddress dq 0
Psf1FontAddress equ __Psf1FontAddress + BOOT_PARTITION_BASE_ADDRESS
dq 0
__VirtualBufferSize dq 0
VirtualBufferSize equ __VirtualBufferSize + BOOT_PARTITION_BASE_ADDRESS
dq 0
__ImageBase dq 0
ImageBase equ __ImageBase + BOOT_PARTITION_BASE_ADDRESS
__KeImageNumSections dw 0
KeImageNumSections equ __KeImageNumSections + BOOT_PARTITION_BASE_ADDRESS
__InitData dq 0
InitData equ __InitData + BOOT_PARTITION_BASE_ADDRESS
__FileImportTable dq 0
FileImportTable equ __FileImportTable + BOOT_PARTITION_BASE_ADDRESS
__KeSectionTable dq 0
KeSectionTable equ __KeSectionTable + BOOT_PARTITION_BASE_ADDRESS
__KernelPeHdrAddress dq 0
KernelPeHdrAddress equ __KernelPeHdrAddress + BOOT_PARTITION_BASE_ADDRESS
; Boot Manager Routine Tables

align 0x40

DiskLbaPacket:
    .PacketSize db 0x10
    db 0
    .NumSectors dw 0
    .RealModeAddress dw DiskTransferBuffer
    .Keep0 dw 0 ; Transfer Buffer 0:DiskTransferBuffer
    .Lba dq 0
    ; 64 bit address of disk transfer buffer
    dq DiskTransferBuffer

align 0x10
VbeReady dq 0 ; To check if VBE is supported and enabled
VbeMode dq 0
align 0x200

VbeInfoBlock:
    .VbeSignature dd 'VBE2'
    .VbeVersion dw 0
    .OemStringPtr times 2 dw 0
    .Capabilities dd 0
    .VideoModePtr times 2 dw 0
    .TotalMemory dw 0


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



DiskTransferBuffer equ 0x2000

stack_top equ 0x7000

ERROR_INVALID_FS_BOOTAREA db "Invalid or Corrupt File System Boot Area", 13, 10, 0
ERROR_UNSUPPORTED_FSBOOT_VERSION db "Unsupported File System Boot Version. BootManager Version : 1.0", 13, 10, 0
StrCPUError db "An unexpected CPU error has occured, halting...", 13, 10, 0

align 0x10
loader_start:
    cli
    mov ax, BOOT_PARTITION_BASE_ADDRESS / 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax

    xor ax, ax
    mov ss, ax
    
    mov sp, stack_top
    mov bp, sp
    
    

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
; 
    ; call SetupVesaVBE

    jmp EnableProtectedMode

.halt:
    hlt
    jmp .halt
; This is the best method availaible on all PC's since 2002 (and some previous ones)

BootLoaderMemory dq 0x800000 ; Used memory by bootloader (0x100000 As initialy used)

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
    cmp esi, 0x100000
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
    ; Check if length of region < 0x100000
    mov esi, [BiosSystemMemoryMap + 8]
    cmp esi, 0x100000
    ja .A000Valid ; BIOS Must map conventionnal memory in the <4GB area or all the memory will be lost
    jmp .SkipMap
    .A000Valid:

    mov dword [BiosSystemMemoryMap], 0x100000 ; 0-0x100000 Reserved for System
    sub dword [BiosSystemMemoryMap + 8], 0x100000
    jmp .FinishSystemCheck
    
GetMemoryMap:
    ; First Call
    mov eax, 0xE820
    xor ebx, ebx ; Continuation Value
    mov edx, 0x534D4150 ; Magic Number
    mov ecx, 24
    mov di, BiosSystemMemoryMap
    ; Set ignore bit if ACPI Field is not supported by BIOS
    mov word [di + 20], 1

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
        mov word [di + 20], 1

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

CheckA20:
    push es
    mov ax, 0xFFFF
    mov es, ax

    xor ax, ax
    mov fs, ax

    ; DS Must be cleared
    ; Write address 0x100500 (0xFFFF:0x510) And 0x500
    mov word [fs:0x500], 1
    mov word [es:0x510], 2 ; 0x100500
    pop es
    cmp word [fs:0x500], 2 ; Writing to 0x100500 without A20 Will write to 0x500
    je .Return0
    clc ; Clear carry flag
    ret
.Return0:
    stc ; Set carry flag
    ret


EnableA20:
    ; Testing A20 Line
    call CheckA20
    jc .BiosEnableA20
    ret ; Otherwise, A20 is already enabled
.BiosEnableA20:
    mov ax, 0x2403
    int 15h ; Query A20 Gate Support
    jc .KeyboardCtlEnableA20 ; if CF Then BIOS Enabling is not supported
    test ah, ah
    jnz .KeyboardCtlEnableA20 ; BIOS A20 Enabling not supported
    test bx, 2 ; Fast A20 Supported (Bit 1 of I/O Port 92h)
    jnz .FastA20Enable
    ; First try to enable using BIOS
    mov ax, 0x2401
    int 15h
    jc .KeyboardCtlEnableA20 ; Function failed
 
    call CheckA20
    jc .KeyboardCtlEnableA20
 
    ret ; A20 Successfully enabled
.KeyboardCtlEnableA20:
    call .A20Wait
    mov al, 0xAD
    out 0x64, al
    call .A20Wait
    mov al, 0xD0
    out 0x64, al
    call .A20Wait2
    in al, 0x60
    push eax
    call .A20Wait
    mov al, 0xD1
    out 0x64, al
    call .A20Wait
    pop eax
    or al, 2
    out 0x60, al
    call .A20Wait
    mov al, 0xAE
    out 0x64, al
    call .A20Wait
 
    call CheckA20
    jc .FastA20Enable
    ret ; A20 Enable successfully
.A20Wait:
    in al, 0x64
    test al, 2
    jnz .A20Wait
    ret
.A20Wait2:
    in al, 0x64
    test al, 1
    jz .A20Wait2
    ret
.FastA20Enable:
    in al, 0x92
    or al, 2
    out 0x92, al
    call CheckA20
    jc .A20IsNotSupported ; Final try and no A20
    ret
.A20IsNotSupported:
    mov eax, 0xdeadcafe
    hlt
    jmp .A20IsNotSupported

[BITS 16]

EnableProtectedMode:


    xor ax,ax
    push ax
    popf
    mov es, ax

    mov ax, 0x1FE0
    mov ds, ax

    lgdt [GDT_REGISTER]

    lidt [IDT_REGISTER]
    mov eax, cr0
    and eax, ~((3 << 2) | (3 << 30))
    or eax, 3 ; Set Protected Mode enable | Monitor coprocessor
    mov cr0, eax

    jmp 0x08:ProtectedModeEntry

ReEnterProtectedMode:
    
    xor ax, ax
    mov es, ax

    mov ax, 0x1FE0
    mov ds, ax

    
    lgdt [GDT_REGISTER]
    mov eax, cr0
    and eax, ~((3 << 2) | (3 << 30))
    or eax, 3 ; Set Protected Mode enable | Monitor coprocessor
    mov cr0, eax

    jmp 0x08:SetupSegmentsAndRet


_print16:
    
    push ax
    push di
    mov ax, 0xe00
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
        ret
        cli
    .halt:
        hlt
        jmp .halt
align 0x10
GDT_REGISTER:
    dw GDT_END - GDT - 1
    dq GDT + BOOT_PARTITION_BASE_ADDRESS

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
        dw 0xFE00
        db 0x1
        db 10011010b
        db 11001111b
        db 0
    .KernelData:
        dw 0xffff
        dw 0xFE00
        db 0x1
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
    .KernelPhysicalMemoryAccessSegment:
        dw 0xffff
        dw 0
        db 0
        db 10010010b
        db 11001111b
        db 0
GDT_END:


SetupVesaVBE:
    ; Get info block
 
    pusha

    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ax, 0x4F00
    mov di, VbeInfoBlock
    push ds
    push es
    push gs
    push fs

    int 0x10

    pop fs
    pop gs
    pop es
    pop ds

    cmp ax, 0x4F ; With Status = 0
    mov di, VesaVbeNotSupported
    jne SetFailureMessage

    
    cmp dword [VbeInfoBlock.VbeSignature], 'VESA'
    jne SetFailureMessage
    xor bx, bx
    mov bx, [VbeInfoBlock.VideoModePtr]; Count for loop
    mov gs, [VbeInfoBlock.VideoModePtr + 2]
    
    .loop:
        mov eax, 0x4F01
        mov cx, [gs:bx]
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
        mov ax, 0x4F02
        mov bx, [VbeMode]
        or bx, 0x4000 ; To use the frame buffer
        xor di, di
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
    

    jmp BOOT_PARTITION_BASE_ADDRESS/0x10:RealModeEntry



RealModeEntry:

    mov ax, 0x1FE0
    mov ds, ax
    xor ax, ax
    mov ss, ax
    mov es, ax
    mov gs, ax
    mov fs, ax


    mov ax, [RealModeFunction]
    call ax

    jmp ReEnterProtectedMode

[BITS 16]

Int13:
    mov si, DiskLbaPacket
    mov ax, 0x4200
    mov dl, [BootDrive]
    int 0x13
    jc .err

    ret
.err:
    mov cx, [DiskLbaPacket.NumSectors]
    mov bx, [DiskLbaPacket.PacketSize]
    and ebx, 0xffff
    and ecx, 0xffff
    mov edi, [DiskLbaPacket.Lba]
    ; mov ax, 0xbbb
    jmp $
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
    mov dword [RealModeReturn], %2
    jmp 0x18:Protected16BitEnterRealMode + BOOT_PARTITION_BASE_ADDRESS
%endmacro

; Last call to return to protected mode
SetupSegmentsAndRet:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax

    mov ax, 0x30
    mov ss, ax
    mov gs, ax

    mov eax, [RealModeCallEaxSaved]
    jmp [RealModeReturn]



align 0x10
ProtectedModeEntry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax

    mov ax, 0x30
    mov ss, ax
    mov gs, ax

    mov esp, stack_top
    mov ebp, esp
   
    ; Check BOOT_POINTER_TABLE

    cmp dword [__BootPointerTable], BOOT_POINTER_TABLE_MAGIC
    jne .InvalidFsArea
    cmp dword [__BootPointerTable + 4], BOOT_POINTER_TABLE_STRMAGIC
    jne .InvalidFsArea
    cmp word [__BootPointerTable + 8], BOOT_MAJOR
    jne .UnsupportedFSBootVersion
    cmp word [__BootPointerTable + 10], BOOT_MINOR
    jne .UnsupportedFSBootVersion


    mov eax, [MemoryMap.Count]
    mov ebx, MemoryMap.MemoryDescriptors


    ; Load Kernel & Other files as well (R15 = Cluster Size)
    mov dl, [0xD] ; Cluster Size of the partition
    and edx, 0xFF ; Get lower bytes only

    mov eax, [__BootPointerTable + BPTOFF_KERNEL_FILE_NUM_CLUSTERS]
    mul edx ; Cluster Size
    shl eax, 9 ; Multiply by 512
    mov ecx, eax
    shr ecx, 12 ; Divide by 12
    inc ecx ; 1 Additionnal Page
    call AllocatePages32
    mov [__KernelBufferAddress], eax ; GS (Base 0) : Kernel Buffer Address
    mov ebx, eax

    mov eax, [__BootPointerTable + BPTOFF_ALLOCATION_TABLE_LBA]
    mov [__AllocationTableLba], eax
    mov eax, [__BootPointerTable + BPTOFF_DATA_CLUSTERS_LBA]
    mov [__DataClustersStartLba], eax

    call LoadPsf1Font


    mov ebx, [__BootPointerTable + BPTOFF_KERNEL_CLUSTER_START]
    mov eax, [__KernelBufferAddress]
    call ReadClusters

; Parsing Kernel File
    mov eax, [__KernelBufferAddress]


    cmp word [gs:eax], 'MZ'
    jne .InvalidKernelImage
    ; Get PE Header Offset
    mov edx, [gs:eax + 0x3C]
    add eax, edx
    ; Check Kernel Image Header
    cmp dword [gs:eax], 'PE' ; PE\0\0
    jne .InvalidKernelImage

    mov [__KernelPeHdrAddress], eax

    cmp word [gs:eax + 4], 0x8664 ; Machine Type
    jne .InvalidKernelImage

    ; OPTIONNAL_HEADER
    cmp word [gs:eax + 24], 0x20b ; PE32+ Magic
    jne .InvalidKernelImage
    cmp word [gs:eax + 0x5C], 1 ; Subsystem NATIVE
    jne .InvalidKernelImage
    mov dx, [gs:eax + 0x5E]
    test dl, 0x40
    jz .InvalidKernelImage ; Must have IMAGE_DYNAMIC_BASE
    
    mov dx, [gs:eax + 22]
    test dl, 0x20 ; Must have LARGE_ADDRESS_AWARE
    jz .InvalidKernelImage

    ; Parse Sections
    mov dx, [gs:eax + 6] ; Num Sections
    mov [__KeImageNumSections], dx
    xor edx, edx
    mov dx, [gs:eax + 20]
    lea eax, [eax + 24 + edx]
    ; Get virtual buffer size
    mov [__KeSectionTable], eax
    mov edx, eax
    xor ecx, ecx
    mov cx, [__KeImageNumSections]

    
; PATTERN : Lx (Loop x), Rx (Return x), Jx (Jmp x) Ax (Alternative JMP x)
; e.g. L1J2 (Loop 1 Jmp 2) L1R4 (Loop 1 Return 4)
.L0: ; Loop 0
    test cx, cx
    jz .E0 ; Exit 0
    ; test SECTION_CHARACTERISTICS

    test dword [gs:edx + 36], 0x20 | 0x40 | 0x80 ; PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA
    jz .L0R1

    mov ebx, [gs:edx + 8] ; Virtual Size
    cmp [gs:edx + 0x10], ebx ; Sizeof Raw Data
    ja .L0J0
.L0R0:
    mov esi, [gs:edx + 0xC] ; Virtual Address
    add ebx, esi
    cmp [__VirtualBufferSize], ebx
    jb .L0J1
.L0R1:
    dec cx
    add edx, 40 ; Sizeof PE_SECTION_TABLE
    jmp .L0
.L0J0:
    mov ebx, [gs:edx + 0x10]
    mov [gs:edx + 8], ebx
    jmp .L0R0
.L0J1:
    mov [__VirtualBufferSize], ebx

    jmp .L0R1
.E0:


    add dword [__VirtualBufferSize], 0x20000 ; Padding

    mov eax, [__VirtualBufferSize]
    mov ecx, eax
    shr ecx, 12
    call AllocatePages32
    mov [__ImageBase], eax

    mov edx, [__KeSectionTable]
    mov cx, [__KeImageNumSections]
    

    mov ebx, [__VirtualBufferSize]



; Load Virtual Buffer

.L1:
    test cx, cx
    jz .E1
    ; test SECTION_CHARACTERISTICS
    test dword [gs:edx + 36], 0x20 | 0x40 | 0x80 ; PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA
    jz .L1J0
    ; Copy Initialized Data
    push ecx
    mov esi, [gs:edx + 20]
    add esi, [__KernelBufferAddress]
    mov edi, [edx + 0xC] ; Destination
    add edi, [ImageBase]
    mov ecx, [edx + 0x10]
    test ecx, 7 ; Test Alignment
    jz .L1A0
    add ecx, 8
    and ecx, ~7
.L1A0:

    shr ecx, 3 ; Divide by 8
    rep movsq
;     ; Set Uninitialized data to 0
    mov ecx, [gs:edx + 8] ; Virtual Size
    sub ecx, [gs:edx + 0x10] ; Sizeof Raw Data
    test ecx, 7
    jz .L1A1
    add ecx, 8 ; Align
    and ecx, ~7
.L1A1:
    mov edi, [gs:edx + 0xC] ; Virtual Address
    add edi, [__ImageBase]
    mov esi, [rdx + 0x10]
    add rdi, rsi
    xor rax, rax
    shr ecx, 3 ; Divide by 8
    rep stosq ; Clear Data
    
    pop rcx

; INITDATA & FIMPORT Must be valid (0x20|0x40|0x80) Data
    ; NASM Uses DWORD String IMMEDIATE (MAX)
    cmp dword [rdx], 'INIT'
    jne .L1A2
    cmp dword [rdx + 4], 'DATA'
    jne .L1A2
    mov edi, [rdx + 0xC]
    add rdi, [ImageBase]
    mov [InitData], rdi
    jmp .L1J0 ; Skip FIMPORT Check
.L1A2:
    cmp dword [rdx], 'FIMP'
    jne .L1J0
    cmp dword [rdx + 4], 'ORT'
    jne .L1J0
    mov edi, [rdx + 0xC]
    add rdi, [ImageBase]
    mov [FileImportTable], rdi
.L1J0:
    dec cx
    add rdx, 40
    jmp .L1
.E1:

cmp qword [InitData], 0
je .InvalidKernelImage
cmp qword [FileImportTable], 0
je .InvalidKernelImage



    mov edx, 0xfafa
    jmp $


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
.InvalidKernelImage:
    mov di, StrInvalidKernelImage
    call _print
    jmp halt


NewLine db 13, 10, 0




.error:
    mov ebx, FailedToGetMemoryMap
    call _print
    .halt:
        hlt
        jmp .halt

PathPsf1Font dw __utf16__("OS\Fonts\zap-light16.psf"), 0
LenPathPsf1Font equ (($ - PathPsf1Font - 1) / 2)

LoadPsf1Font:
    mov ebx, PathPsf1Font
    mov eax, LenPathPsf1Font


    call LoadBootPointerFile
    mov [__Psf1FontAddress], eax
    ; Check Magic 0 & Magic 1
    cmp byte [gs:eax], 0x36 ; PSF1_MAGIC0
    jne FailedToLoadResources
    cmp byte [gs:eax + 1], 0x04 ; PSF1_MAGIC1
    jne FailedToLoadResources
    ret ; Font Loaded Successfully


FailedToLoadResources:
    mov di, StrFailedToLoadResources
    call _print

    jmp halt

LoadBootPointerFile:
    push edx
    mov edx, [__BootPointerTable + BPTOFF_ENTRIES_OFFSET]
    add edx, __BootPointerTable
    mov esi, [__BootPointerTable + BPTOFF_ENTRY_SIZE]
    mov ecx, [__BootPointerTable + BPTOFF_NUM_ENTRIES]
.loop:
    test ecx, ecx
    jz FailedToLoadResources
    cmp [edx + BPT_ENTRY_PATH_LENGTH], ax
    jne .ContinueSearch1
    ; Cmp Str

    push ecx
    push edx
    push ebx
    lea ecx, [edx + BPT_ENTRY_PATH]
    push dword 0; CMP_COUNT
    .CmpLoop:
        cmp [esp], eax
        je .ValidPath
        mov dx, [ecx]
        mov di, [ebx]

        ; Convert to lowercase if possible
        cmp di, 'A'
        jb .R0
        cmp di, 'Z'
        jbe .r10tolowercase
        .R0:
        cmp dx, 'A'
        jb .R1
        cmp dx, 'Z'
        jbe .r13tolowercase
        .R1:
        cmp di, dx
        jne .ContinueSearch ; Invalid Path
        add ebx, 2
        add ecx, 2
        inc dword [esp] ; Index
        jmp .CmpLoop
    .ValidPath:
        add esp, 0x8 ; No pop needed, registers will be replaced
        pop edx
        add esp, 4
        mov ecx, [edx + BPT_ENTRY_NUM_CLUSTERS]
        mov eax, ecx
        push edx
        mul byte [0xD] ; BOOT_PARTITION_BASE_ADDR + 0xD
        pop edx
        mov ecx, eax
        call AllocatePages32
        push eax
        mov ebx, [edx + BPT_ENTRY_CLUSTER_START]
        ; File Size in cluster bytes
        push edx
        mov eax, [edx + BPT_ENTRY_NUM_CLUSTERS]
        mov ecx, [0xD]
        and ecx, 0xFF
        mul ecx
        mov ecx, eax
        shl ecx, 9
        pop edx
        pop eax
        call ReadClusters
        
        pop edx
        ret
.ContinueSearch:
    add esp, 4 ; POP Counter
    pop ebx
    pop edx
    pop ecx
    mov eax, 0xbbb
    jmp $

.ContinueSearch1:
    dec ecx
    add edx, esi
    jmp .loop

.r10tolowercase:
    add di, 0x20
    jmp .R0
.r13tolowercase:
    add dx, 0x20
    jmp .R1



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
    ; Calculate page count as lower ram (for e.g. if page count = 0x10000000 it is considered 0)
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
times 0x12000 - ($-$$) db 0



; Basic 4MB Page table to enter long mode
PageTable:
.Pml4:
dq .Pdp + BOOT_PARTITION_BASE_ADDRESS + 3 ; + 3 (or | 3 ) which set Present & R/W Bits
times 511 dq 0
.Pdp:
dq .Pd + BOOT_PARTITION_BASE_ADDRESS + 3
times 511 dq 0
.Pd: ; Map low 4MB
dq 0x83
dq 0x200083
dq 0x400083
dq 0x600083
dq 0x800083

times 512 dq 0
; .Pt:
; %assign i 0
; %rep 512
; dq i + 3 ; Physical Address of Page | 3 (Present & R/W)
; %assign i i + 0x1000
; %endrep

align 0x10
__ClusterChainSector times 0x200 db 0
ClusterChainSector equ __ClusterChainSector + BOOT_PARTITION_BASE_ADDRESS
BOOT_MAJOR equ 1
BOOT_MINOR equ 0


[BITS 32]
; EBX = Cluster Start (Current Cluster), EAX = Buffer
ReadClusters:
pusha


mov edi, [0xD] ; Cluster Size (in Sectors)
and edi, 0xFF
mov esi, edi
shl esi, 9 ; Multiply by 512
; Read First Cluster

push ebx
; Construct Physical Cluster Address
push eax
mov eax, ebx
mul edi
mov ebx, eax
pop eax
add ebx, [__DataClustersStartLba]

mov ecx, edi ; Cluster Size
mov edi, eax
call DiskRead
add eax, esi ; Increment Buffer Pointer
pop ebx

mov ecx, [__LastClusterChainSector]

.loop:

    mov edx, ebx
    shr ebx, 7
    and edx, 0x7F
    shl edx, 2
    cmp ecx, ebx
    jne .ReadAllocationSector
.R0:


    lea edx, [__ClusterChainSector + edx]
    mov edx, [edx]
    cmp edx, CLUSTER_END_OF_CHAIN
    je .Exit

;     ; Otherwise read cluster
;     ; Construct Physical Cluster Address
    push eax
    push edx ; RDX Used on mul instruction

    mov eax, edx
    mov dl, [0xD]
    and edx, 0xFF
    mul edx
    pop edx ; EAX = EDX * [CLUSTER_SIZE] = CLUSTER * CLUSTER_SIZE
    
    ; EBX = LBA_START
    mov ebx, eax
    pop eax
    add ebx, [__DataClustersStartLba]
    push ecx
    mov cl, [0xD] ; Cluster Size
    and ecx, 0xFF
    mov edi, eax
    call DiskRead
    shl ecx, 9
    add eax, ecx ; Increment Buffer Pointer
    pop ecx
    mov ebx, edx
    jmp .loop

.ReadAllocationSector:
    push edx
    push ebx
    mov edx, [__AllocationTableLba]
    add edx, ebx
    mov ecx, ebx
    mov ebx, edx
    push ecx

    mov ecx, 1 ; 1 Sector
    mov edi, __ClusterChainSector + BOOT_PARTITION_BASE_ADDRESS
    call DiskRead
    pop ecx
    pop ebx
    pop edx
    jmp .R0
.Exit:

    mov [__LastClusterChainSector], ecx

    popa
    ret


; RCX = num sectors, RDI = buffer (Physical Address), RBX = LBA
DiskRead:
    ; calculate full read count
    pusha
    mov eax, ecx
    and eax, 7
    shr ecx, 3 ; divide by 3

    
    .loop:
        test ecx, ecx
        jz .exit1
        ; do int 13
        mov word [DiskLbaPacket.NumSectors], 8
        mov [DiskLbaPacket.Lba], ebx
        mov word [DiskLbaPacket.RealModeAddress], DiskTransferBuffer
        push ecx
        push edi
        push ebx
        
        CallRealMode Int13, .R0
.R0:
        ; Copy content into rdi
        mov ecx, 0x400 ; 4 dword * 0x800 = 0x1000
        push ds
        mov ax, 0x30
        mov ds, ax
        mov es, ax

        mov esi, DiskTransferBuffer
        mov edi, [esp + 4]
        rep movsd ; EDI, ESI Incremented by 0x1000
        pop ds
        pop ebx
        add esp, 4 ; EDI ALREADY POPPED
        pop ecx
        add ebx, 8
        dec ecx
        jmp .loop
    .exit1:
    test eax, eax
    jz .NoMoreSectors
    ; Setup packet

    mov [DiskLbaPacket.NumSectors], ax
    mov [DiskLbaPacket.Lba], ebx
    
    mov word [DiskLbaPacket.RealModeAddress], DiskTransferBuffer
    
    CallRealMode Int13, .R1
    
.R1:
    ; Copy content into edi
    mov ecx, eax ; 4 dword * 0x800 = 0x1000
    shl ecx, 9 ; * 512 / 4 or << 9 >> 2
    push ds
    mov ax, 0x30
    mov ds, ax
    mov es, ax
    mov esi, DiskTransferBuffer
    shr ecx, 2 ; Divide by 8
    ; EDI Already present
    rep movsd
    pop ds
    .NoMoreSectors:

    
    popa
    ret

_tohex32:
    ; Save registers
    pusha

    xor ecx, ecx ; size variable
    mov edx, eax ; ValTmp  
.SizeLoop:
    inc ecx
    shr edx, 4
    test edx, edx
    jz .ConvertNumber
    jmp .SizeLoop
.ConvertNumber:
    xor edi, edi ; i = 0; i < size; i++
    
    lea esi, [HexBuffer - 1 + ecx] ; _Buffer += size - 1
.ConvertLoop:
    cmp edi, ecx
    je .Exit
    inc edi
    
    mov bl, al ; unsigned char c = _Value & 0xF
    and bl, 0xF
    cmp bl, 0xA
    jb .ZeroPlusC ; _Buffer[size - (i + 1)] = '0' + c;
    jmp .A_PlusC
    .ConvertCharExit:
    shr eax, 4
    dec esi
    jmp .ConvertLoop
.ZeroPlusC:
    add bl, '0'
    mov [esi], bl
    jmp .ConvertCharExit
.A_PlusC:
    sub bl, 0xA
    add bl, 'A'
    mov [esi], bl
    jmp .ConvertCharExit
.Exit:
    lea esi, [HexBuffer + ecx]
    mov byte [esi], 0 ; Ending Character /0

    
    popa
    ret


times BOOTMGR_SIZE - ($-$$) db 0

BootPointerTable equ __BootPointerTable + BOOT_PARTITION_BASE_ADDRESS

__BootPointerTable: ; Setup in FS To eliminate FS & Disk drivers usage and acquire faster load performance 

%include "bptdefs.asm"