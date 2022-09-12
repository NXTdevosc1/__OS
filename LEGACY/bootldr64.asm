

[BITS 32]
CheckCpuid:
    pushf
    or dword [esp], 0x200000 ; Set ID Bit
    popf
    pushf
    pop eax
    test eax, 0x200000
    jz .CpuidNotSupported
    ret
.CpuidNotSupported:
    mov di, StrCpuidNotSupported
    call _print
    jmp halt

CheckLongModeSupport:
    ; Test Extended Function Availability
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .LongModeNotSupported ; Extended Function not availaible, therefore no long mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29 ; Long Mode Support Bit
    jz .LongModeNotSupported
    ret ; Otherwise long mode is supported

.LongModeNotSupported:
    mov di, StrLongModeNotSupported
    call _print
    jmp halt
EnterLongMode:
    call CheckCpuid
    call CheckLongModeSupport


    ; Active PAE & Page Global (PGE)
    mov eax, (1 << 5) | (1 << 7)
    mov cr4, eax


    ; Set CR3
    mov eax, PageTable + BOOT_PARTITION_BASE_ADDRESS
    mov cr3, eax


    ; Activate Long Mode
    mov ecx, 0xC0000080 ; EFER_MSR
    rdmsr
    or eax, 1 << 8 ; Set LME (Long Mode Enable) bit
    wrmsr

        
    lidt [IDTR64 + BOOT_PARTITION_BASE_ADDRESS]

    ; Set CR0 With (PE, MP, PG)

    mov eax, 3 | (1 << 31)
    mov cr0, eax
 

    jmp 0x20:LongModeEntry + BOOT_PARTITION_BASE_ADDRESS ; Jmp to long mode

StrCpuidNotSupported db "The bootloader cannot continue, Processor does not support CPUID.", 13, 10, 0
StrLongModeNotSupported db "The bootloader cannot continue, Long Mode (x64 A.K.A 64 BIT Mode) Not Supported.", 13, 10, 0


[BITS 64]
SavedRAX dq 0
%macro CallRealMode64 2
    mov [SavedRAX + BOOT_PARTITION_BASE_ADDRESS], rax
    push 0x8
    push %%protectedmode

    retfq
[BITS 32]
%%protectedmode:

    mov ax, 0x10
    mov ds, ax
    mov es, ax

    
    mov ax, 0x30
    mov ss, ax
    
    
    mov eax, 3
    mov cr0, eax ; This will automatically disabled LME (And compatibility mode also)
    
    xor eax, eax
    mov cr4, eax

    
    CallRealMode %1, %%tolongmode
%%tolongmode:
; Active PAE & Page Global (PGE)
    
    mov eax, PageTable + BOOT_PARTITION_BASE_ADDRESS
    mov cr3, eax

    mov eax, (1 << 5) | (1 << 7)
    mov cr4, eax

    push ecx
    push edx
    ; Activate Long Mode
    mov ecx, 0xC0000080 ; EFER_MSR
    rdmsr
    or eax, 1 << 8 ; Set LME (Long Mode Enable) bit
    wrmsr
    pop edx
    pop ecx
    mov eax, 3 | (1 << 31) ; Set PE | MP | PG Bits
    mov cr0, eax

    lidt [IDTR64 + BOOT_PARTITION_BASE_ADDRESS]

    
    jmp 0x20:%%exit + BOOT_PARTITION_BASE_ADDRESS

[BITS 64]
%%exit:

mov ax, 0x28
mov ds, ax
mov ss, ax
mov es, ax

mov rax, [SavedRAX + BOOT_PARTITION_BASE_ADDRESS]
jmp %2
%endmacro


_print64:
    CallRealMode64 _print16, .r
    .r:
    ret

LongModeEntry:
    mov ax, 0x28
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    
    ; Clear RFLAGS
    xor rax, rax
    push rax
    popf

    mov rax, [MemoryMap.Count + BOOT_PARTITION_BASE_ADDRESS]
    mov rbx, MemoryMap.MemoryDescriptors + BOOT_PARTITION_BASE_ADDRESS

    ; Reset unaccessible registers in Real Mode

    .loop:
        test rax, rax
        jz .end0

        mov dl, [rbx]
        

        cmp dl, 1 ; Conventionnal Memory
        jne .Skip0
        push rbx
        mov rdx, rbx
        mov rbx, [rdx + 9]
        mov rcx, [rdx + 1]
        call IdentityMapPages
        pop rbx
        .Skip0:

        add rbx, SIZE_KERNEL_MEMORY_MAP
        dec rax
        jmp .loop

    .end0:

    ; Load Kernel & Other files as well (R15 = Cluster Size)
    mov r15, [BOOT_PARTITION_BASE_ADDRESS + 13] ; Cluster Size of the partition
    and r15, 0xFF ; Get lower bytes only

    mov eax, [BootPointerTable + BPTOFF_KERNEL_FILE_NUM_CLUSTERS]
    mul r15 ; Cluster Size
    shl rax, 9 ; Multiply by 512
    mov rcx, rax
    shr rcx, 12 ; Divide by 12
    inc rcx ; 1 Additionnal Page

    
    call AllocatePages
    mov [KernelBufferAddress], rax
    mov rbx, rax


    
    ; RBX = Cluster Start (Current Cluster), R15 = Buffer
    mov rax, [BootPointerTable + BPTOFF_ALLOCATION_TABLE_LBA]
    mov [AllocationTableLba], rax
    mov rax, [BootPointerTable + BPTOFF_DATA_CLUSTERS_LBA]
    mov [DataClustersStartLba], rax
    
    call LoadPsf1Font


    mov rbx, [BootPointerTable + BPTOFF_KERNEL_CLUSTER_START]


    mov r15, [KernelBufferAddress]
    call ReadClusters

; Parsing Kernel File
    mov rax, [KernelBufferAddress]
    cmp word [rax], 'MZ'
    jne .InvalidKernelImage
    ; Get PE Header Offset
    mov edx, [rax + 0x3C]
    add rax, rdx
    ; Check Kernel Image Header
    cmp dword [rax], 'PE' ; PE\0\0
    jne .InvalidKernelImage

    mov [KernelPeHdrAddress], rax

    cmp word [rax + 4], 0x8664 ; Machine Type
    jne .InvalidKernelImage

    ; OPTIONNAL_HEADER
    cmp word [rax + 24], 0x20b ; PE32+ Magic
    jne .InvalidKernelImage

    cmp word [rax + 0x5C], 1 ; Subsystem NATIVE
    jne .InvalidKernelImage
    mov dx, [rax + 0x5E]
    test dl, 0x40
    jz .InvalidKernelImage ; Must have IMAGE_DYNAMIC_BASE
    
    mov dx, [rax + 22]
    test dl, 0x20 ; Must have LARGE_ADDRESS_AWARE
    jz .InvalidKernelImage

    ; Parse Sections
    mov dx, [rax + 6] ; Num Sections
    mov [KeImageNumSections], dx
    xor rdx, rdx
    mov dx, [rax + 20]
    lea rax, [rax + 24 + rdx]
    ; Get virtual buffer size
    mov [KeSectionTable], rax
    mov rdx, rax
    xor rcx, rcx
    mov cx, [KeImageNumSections]

    

; PATTERN : Lx (Loop x), Rx (Return x), Jx (Jmp x) Ax (Alternative JMP x)
; e.g. L1J2 (Loop 1 Jmp 2) L1R4 (Loop 1 Return 4)
.L0: ; Loop 0
    test cx, cx
    jz .E0 ; Exit 0
    ; test SECTION_CHARACTERISTICS

    test dword [rdx + 36], 0x20 | 0x40 | 0x80 ; PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA
    jz .L0R1

    mov ebx, [rdx + 8] ; Virtual Size
    cmp [rdx + 0x10], ebx ; Sizeof Raw Data
    ja .L0J0
.L0R0:
    mov r8d, [rdx + 0xC] ; Virtual Address
    add rbx, r8
    cmp [VirtualBufferSize], rbx
    jb .L0J1
.L0R1:
    dec cx
    add rdx, 40 ; Sizeof PE_SECTION_TABLE
    jmp .L0
.L0J0:
    mov ebx, [rdx + 0x10]
    mov [rdx + 8], ebx
    jmp .L0R0
.L0J1:
    mov [VirtualBufferSize], rbx

    jmp .L0R1
.E0:

    add qword [VirtualBufferSize], 0x20000 ; Padding

    mov rax, [VirtualBufferSize]
    mov rcx, rax
    shr rcx, 12
    call AllocatePages
    mov [ImageBase], rax

    mov rdx, [KeSectionTable]
    mov cx, [KeImageNumSections]

; Load Virtual Buffer

.L1:
    test cx, cx
    jz .E1
    ; test SECTION_CHARACTERISTICS
    test dword [rdx + 36], 0x20 | 0x40 | 0x80 ; PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA
    jz .L1J0
    ; Copy Initialized Data
    push rcx
    mov esi, [rdx + 20]
    add rsi, [KernelBufferAddress]
    mov edi, [rdx + 0xC] ; Destination
    add rdi, [ImageBase]
    mov ecx, [rdx + 0x10]
    test ecx, 7 ; Test Alignment
    jz .L1A0
    add rcx, 8
    and ecx, ~7
.L1A0:

    shr ecx, 3 ; Divide by 8
    rep movsq
;     ; Set Uninitialized data to 0
    mov ecx, [rdx + 8] ; Virtual Size
    sub ecx, [rdx + 0x10] ; Sizeof Raw Data
    test ecx, 7
    jz .L1A1
    add rcx, 8 ; Align
    and ecx, ~7
.L1A1:
    mov edi, [rdx + 0xC] ; Virtual Address
    add rdi, [ImageBase]
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
    
; Relocate Kernel Image

mov rax, [KernelPeHdrAddress]
add rax, 176 ; Base Relocation Table

; StartBlock
mov ebx, [rax]
add rbx, [ImageBase]

; EndSection
mov ecx, [rax + 4]
add rcx, rbx

xor r9, r9 ; Keep high 32 bits clear

; Get PE_IMAGE_BASE
mov r15, [KernelPeHdrAddress]
mov r15, [r15 + 48] ; Image Base
.L2:
    ; while(NextBlock <= SectionEnd)
    cmp rbx, rcx
    jae .E2
	; if (!RelocationBlock->PageRva || !RelocationBlock->BlockSize) break;

    cmp dword [rbx], 0
    je .E2
    cmp dword [rbx + 4], 0
    je .E2
    
.A0:
    ; Get Num Entries
    mov r8d, [rbx + 4] ; Block Size
    sub r8d, 8 ; sizeof RelocationBlock
    shr r8d, 1 ; Divide By 2 (Relocation Entry Size)
    
    lea rdx, [rbx + 8] ; Relocation Entries Start
    ; R8 = Num Entries (Decrement)
.L2L1:
    test r8d, r8d
    jz .L2E0
    mov r9w, [rdx]
    ; Compare Type
    mov r10w, r9w

    shr r10w, 12 ; Get Type
    cmp r10w, 10 ; IMAGE_REL_BASED_DIR64
    jne .L2L1A0
    ; Get Target rebase address
    mov r11d, [rbx] ; Page RVA
    mov r10, [ImageBase]
    add r10, r11
    and r9, 0xFFF ; GetOffset
    add r10, r9 ; Offset
    
    ; Calculate Relocated Address
    mov rax, [r10]
    sub rax, r15 ; Substract value from image base
    add rax, [ImageBase]
    ; Set the new Address
    mov [r10], rax
.L2L1A0:
    add rdx, 2
    dec r8d
    jmp .L2L1

.L2E0:
    mov edx, [rbx + 4]
    add rbx, rdx
    jmp .L2
.E2:

; Load File Import Table

mov r8, 0x14A ; Sizeof FILE_IMPORT_ENTRY
mov rbx, [FileImportTable]


.L3:
    cmp dword [rbx], 0 ; ENDOF_TABLE
    je .E3

    ; Calculate Len Path
    lea rdi, [rbx + 30]
    xor rax, rax
.L3L0:
    cmp word [rdi], 0
    je .L3E0
    inc ax
    add rdi, 2
    jmp .L3L0
.L3E0:

    ; Load Boot Pointer File : RBX = Path, AX = LenPath
    push rbx
    add rbx, 30 ; Path Offset
    push r8
    call LoadBootPointerFile
    pop r8
    pop rbx
    mov [rbx + 4], rcx ; Loaded File Size
    mov [rbx + 0xC], rax ; Loaded File Buffer

    add rbx, r8
    jmp .L3
.E3:

; Initialize VESA/VBE
; CallRealMode64 SetupVesaVBE, .E4

.E4:

; Setup Frame Buffer Descriptor
    mov ax, [VbeModeInfo.Width]
    mov [FrameBufferDescriptor + 4], ax ; Horizontal Resolution
    mov ax, [VbeModeInfo.Height]
    mov [FrameBufferDescriptor + 8], ax ; Vertical Resolution
    mov dword [FrameBufferDescriptor + 0xC], 0x10000 ; Version
    mov eax, [VbeModeInfo.FrameBufferAddress]
    mov ecx, [VbeModeInfo.OffsetScreenMemory]
    add rax, rcx
    mov [FrameBufferDescriptor + 0x10], rax ; FrameBufferBase
    ; Frame Buffer Size
    xor rax, rax
    mov ax, [VbeModeInfo.Pitch]
    mov ecx, [FrameBufferDescriptor + 8]
    mov [FrameBufferDescriptor + 48], rax ; Pitch
    mul rcx
    mov [FrameBufferDescriptor + 0x18], rax ; FrameBufferSize
    
    ; Color Depth, Refresh Rate
    mov ax, [VbeModeInfo.BitsPerPixel]
    mov [FrameBufferDescriptor + 0x20], ax
    mov dword [FrameBufferDescriptor + 0x24], 60 ; 60 FPS (Assumming)



; Setup InitData Structure

; Fill InitData with 0 (Already filled with Magic Number)

mov rdi, [InitData]
mov rcx, 0x20
xor rax, rax
rep stosq

mov rax, [InitData]
mov qword [rax], MemoryMap
mov qword [rax + 8], 1 ; NumFrameBuffer
mov qword [rax + 0x10], FrameBufferDescriptor
mov rbx, [Psf1FontAddress]
mov [rax + 0x18], rbx ; StartFont
mov rbx, [ImageBase]
mov [rax + 0x3C], rbx ; Image Base

mov rbx, [VirtualBufferSize]
mov [rax + 0x44], rbx ; Image Size

; Set PE data directories offset
mov rbx, [KernelPeHdrAddress]

add rbx, 0x88 ; Offset to data directories


mov [rax + 0x58], rbx ; PE Data Directories

mov rax, [KernelPeHdrAddress]
mov eax, [rax + 40] ; EntryPointAddress
add rax, [ImageBase]


; Map Frame Buffer (Temporary for testing)
mov rbx, [FrameBufferDescriptor + 0x10] ; FB Base
mov rcx, [FrameBufferDescriptor + 0x18] ; FB Size
shr rcx, 12 ; Divide by 0x1000
inc rcx ; Padding

call IdentityMapPages



wbinvd


jmp rax

.R0: ; Return Address 0
    cli
    hlt
    jmp .R0
.InvalidKernelImage:
    mov di, .StrInvalidKernelImage
    call _print64
    jmp halt64
.StrInvalidKernelImage db "Invalid Kernel Image, please re-install your Operating System.", 13, 10, 0

%macro UNICODE_LENGTH 1 ; %1 = Unicode Str ($ must be right after Str)
    (($ - %1) / 2) - 1
%endmacro

__PathPsf1Font dw __utf16__("OS\Fonts\zap-light16.psf"), 0
LenPathPsf1Font equ (($ - __PathPsf1Font - 1) / 2)
PathPsf1Font equ __PathPsf1Font + BOOT_PARTITION_BASE_ADDRESS


align 0x10
FrameBufferDescriptor:
    times 0x100 db 0

LoadPsf1Font:
    mov rbx, PathPsf1Font
    mov rax, LenPathPsf1Font


    call LoadBootPointerFile
    mov rcx, 0xba
    jmp $
    mov [Psf1FontAddress], rax
    
    ; Check Magic 0 & Magic 1

    cmp byte [rax], 0x36 ; PSF1_MAGIC0
    jne FailedToLoadResources
    cmp byte [rax + 1], 0x04 ; PSF1_MAGIC1
    jne FailedToLoadResources
    ret ; Font Loaded Successfully
FailedToLoadResources:

    mov di, StrFailedToLoadResources
    CallRealMode64 _print16, halt64

StrFailedToLoadResources db "Failed to load resources. Please re-install your Operating System.", 13, 10, 0

; RBX = Path, AX = LenPath
; FUNCTION : RDX = ADDRESS OF CURRENT ENTRY, R8 = ENTRY_SIZE, RCX = REMAINING_ENTRIES
; RAX = Return Address, RCX = File Size (Num Cluster * Cluster Size)


LoadBootPointerFile:
    push rdx
    mov edx, [BootPointerTable + BPTOFF_ENTRIES_OFFSET]
    add rdx, BootPointerTable
    mov r8d, [BootPointerTable + BPTOFF_ENTRY_SIZE]
    mov ecx, [BootPointerTable + BPTOFF_NUM_ENTRIES]

.loop:
    test ecx, ecx
    jz FailedToLoadResources
    cmp [rdx + BPT_ENTRY_PATH_LENGTH], ax
    jne .ContinueSearch
    ; Cmp Str
    xor r9, r9 ; CMP_COUNT
    mov r11, rbx
    lea r12, [rdx + BPT_ENTRY_PATH]
    .CmpLoop:
        cmp r9, rax
        je .ValidPath
        mov r10w, [r11]
        mov r13w, [r12]
        ; Convert to lowercase if possible
        cmp r10w, 'A'
        jb .R0
        cmp r10w, 'Z'
        jbe .r10tolowercase
        .R0:
        cmp r13w, 'A'
        jb .R1
        cmp r13w, 'Z'
        jbe .r13tolowercase
        .R1:
        cmp r10w, r13w
        jne .ContinueSearch ; Invalid Path
        add r11, 2
        add r12, 2
        inc r9 ; Index
        jmp .CmpLoop
    .ValidPath:

        mov ecx, [rdx + BPT_ENTRY_NUM_CLUSTERS]
        mov eax, ecx
        push rdx
        mul byte [BOOT_PARTITION_BASE_ADDRESS + 0xD]
        pop rdx
        mov rcx, rax
        call AllocatePages
        push rax
        mov r15, rax
        mov rbx, [rdx + BPT_ENTRY_CLUSTER_START]
        ; File Size in cluster bytes
        push rdx
        mov eax, [rdx + BPT_ENTRY_NUM_CLUSTERS]
        mov ecx, [BOOT_PARTITION_BASE_ADDRESS + 0xD]
        and ecx, 0xFF
        mul ecx
        mov rcx, rax
        shl rcx, 9
        pop rdx
        call ReadClusters
        MOV RAX, 0XDEADBEEF
        JMP $
        jmp $
        
        pop rax
        jmp .Exit
.ContinueSearch:
    dec ecx
    add rdx, r8
    jmp .loop
.Exit:
    pop rdx
    ret
.r10tolowercase:
    add r10w, 0x20
    jmp .R0
.r13tolowercase:
    add r13w, 0x20
    jmp .R1



; RBX = Cluster Start (Current Cluster), R15 = Buffer
ReadClusters:
push rax
push rcx
push rdx
push rdi
push rsi
push r8
push r9
push r10
push r11
push r12


mov r8, [LastClusterChainSector]
mov r11, [DataClustersStartLba]
mov r12, [BOOT_PARTITION_BASE_ADDRESS + 0xD] ; Cluster Size (in Sectors)
and r12, 0xFF
mov r13, r12
shl r13, 9 ; Multiply by 512
; Read First Cluster

push rbx
; Construct Physical Cluster Address
mov rax, rbx
mul r12
mov rbx, rax
add rbx, [DataClustersStartLba]

mov rcx, r12 ; Cluster Size
mov rdi, r15
add r15, r13 ; Increment Buffer Pointer
call DiskRead
jmp $
pop rbx


.loop:

    mov r9, rbx
    shr r9, 7
    mov r10, rbx
    and r10, 0x7F
    shl r10, 2
    cmp r8, r9
    jne .ReadAllocationSector
.R0:
    push rax
    mov rax, r9
    call _tohex
    mov di, HexBuffer
    call _print64
    mov di, NewLine
    call _print64
    pop rax

    lea rdx, [ClusterChainSector + r10]
    mov edx, [rdx]
    cmp edx, CLUSTER_END_OF_CHAIN
    je .Exit
    ; Otherwise read cluster
    ; Construct Physical Cluster Address
    push rdx ; RDX Used on mul instruction
    mov rax, rdx
    mul r12
    pop rdx
    mov rbx, rax
    add rbx, r11

    mov rcx, r12 ; Cluster Size
    mov rdi, r15
    add r15, r13 ; Increment Buffer Pointer
    call DiskRead
    mov rbx, rdx
    jmp .loop

.ReadAllocationSector:
    push rbx
    mov rbx, [AllocationTableLba]
    add rbx, r9
    mov r8, r9

    mov rcx, 1 ; 1 Sector
    mov rdi, ClusterChainSector

    call DiskRead
    pop rbx
    jmp .R0
.Exit:

    mov [LastClusterChainSector], r8

    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rax
    ret



; rbx = Physical Address
; rcx = Count
; r8 = PT, R9 = PD, R10 = PDP, R11 = PML4
IdentityMapPages:
    push rax
    push rbx
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
.loop:
    test rcx, rcx
    jz .Finish
; Calculate INDEXES
    ; PT Index
    mov r8, rbx
    shr r8, 12
    and r8, 0x1FF
    ; PD Index
    mov r9, rbx
    shr r9, 21
    and r9, 0x1FF
    ; PDP Index
    mov r10, rbx
    shr r10, 30
    and r10, 0x1FF
    ; PML4 Index
    mov r11, rbx
    shr r11, 39
    and r11, 0x1FF
    ; Multiply Indexes by 8 For fast access
    shl r8, 3
    shl r9, 3
    shl r10, 3
    shl r11, 3
    ; PML4 Allocation
    mov rdx, PageTable + BOOT_PARTITION_BASE_ADDRESS
    add rdx, r11
    test qword [rdx], 1
    jz .AllocatePml4
    .retpml4alloc:
    ; PDP Allocation
    mov rdx, [rdx] ; Get Pointer
    and rdx, ~(0xFFF)
    add rdx, r10
    test qword [rdx], 1
    jz .AllocatePdp
    .retpdpalloc:
    ; PD Allocation
    mov rdx, [rdx]
    and rdx, ~(0xFFF)
    add rdx, r9
    test qword [rdx], 1
    jz .AllocatePd
    .retpdalloc:
    ; Setting Page Table Entry
    mov rdx, [rdx]
    and rdx, ~(0xFFF)
    add rdx, r8
    mov rax, rbx
    or rax, 3 ; Present | RW
    mov [rdx], rax

    ; Continue
    add rbx, 0x1000
    dec rcx
    jmp .loop
.AllocatePml4:
    call .AllocateEntryPage
    jmp .retpml4alloc
.AllocatePdp:
    call .AllocateEntryPage
    jmp .retpdpalloc
.AllocatePd:
    call .AllocateEntryPage
    jmp .retpdalloc
.AllocateEntryPage:
    push rcx
    mov rcx, 1 ; 1 Page
    call AllocatePages
    ; Clear Page
    push rax
    mov rdi, rax
    xor rax, rax
    mov rcx, 0x200 ; 512 QWORDS = 4K
    rep stosq
    pop rax
    pop rcx
    or rax, 3 ; Set Present | RW
    mov [rdx], rax
    ret
.Finish:
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret
; Allocates Constant Pages that cannot be freed up
; rcx = Num Pages
; Return value : rax = Address
; Primarely used for allocating page tables in 32 bits

AllocatePages:
    push rbx
    push rdx
    push rcx
    mov rax, [MemoryMap.Count + BOOT_PARTITION_BASE_ADDRESS]
    mov rbx, MemoryMap.MemoryDescriptors + BOOT_PARTITION_BASE_ADDRESS

.loop:
    test rax, rax
    jz .err ; Return 0
    mov dl, [rbx]
    cmp dl, 1
    jne .ContinueSearch

    cmp [rbx + 1], rcx
    jb .ContinueSearch
    ; Memory Found
    mov rax, [rbx + 9]
    sub [rbx + 1], rcx
    shl rcx, 12
    add [BootLoaderMemory + BOOT_PARTITION_BASE_ADDRESS], rcx
    add [rbx + 9], rcx
    jmp .Exit ; Return Address
    .ContinueSearch:
        add rbx, SIZE_KERNEL_MEMORY_MAP
        dec rax
        jmp .loop
.Exit:
    
    pop rcx
    pop rdx
    pop rbx
    ret ; Return rax
.err:
    mov di, UnsufficientMemory
    call _print64
    jmp halt64

halt64:
    hlt
    jmp halt64
_tohex:
    ; Save registers
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push rbp

    mov rbp, rsp
    xor rcx, rcx ; size variable
    mov rdx, rax ; ValTmp  
.SizeLoop:
    inc rcx
    shr rdx, 4
    test rdx, rdx
    jz .ConvertNumber
    jmp .SizeLoop
.ConvertNumber:
    xor rdi, rdi ; i = 0; i < size; i++
    
    lea rsi, [HexBuffer - 1 + rcx] ; _Buffer += size - 1
.ConvertLoop:
    cmp rdi, rcx
    je .Exit
    inc rdi
    
    mov bl, al ; unsigned char c = _Value & 0xF
    and bl, 0xF
    cmp bl, 0xA
    jb .ZeroPlusC ; _Buffer[size - (i + 1)] = '0' + c;
    jmp .A_PlusC
    .ConvertCharExit:
    shr rax, 4
    dec rsi
    jmp .ConvertLoop
.ZeroPlusC:
    add bl, '0'
    mov [rsi], bl
    jmp .ConvertCharExit
.A_PlusC:
    sub bl, 0xA
    add bl, 'A'
    mov [rsi], bl
    jmp .ConvertCharExit
.Exit:
    lea rsi, [HexBuffer + rcx]
    mov byte [rsi], 0 ; Ending Character /0

    
    pop rbp
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret

UnsufficientMemory db "Memory Allocation Failed, Please upgrade your RAM.", 13, 10, 0


; RCX = num sectors, RDI = buffer, RBX = LBA
DiskRead:

    ; calculate full read count
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi

    mov rax, rcx
    and rax, 7
    shr rcx, 3 ; divide by 3

    
    .loop:
        test rcx, rcx
        jz .exit1
        ; do int 13
        mov word [DiskLbaPacket.NumSectors + BOOT_PARTITION_BASE_ADDRESS], 8
        mov [DiskLbaPacket.Lba + BOOT_PARTITION_BASE_ADDRESS], rbx
        mov word [DiskLbaPacket.RealModeAddress + BOOT_PARTITION_BASE_ADDRESS], DiskTransferBuffer
        push rcx
        push rdi
        push rbx
        push rdi
        
        CallRealMode64 Int13, .R0
.R0:
        ; Copy content into rdi
        mov rcx, 0x200 ; 4 dword * 0x800 = 0x1000
        mov rsi, DiskTransferBuffer
        rep movsq
        pop rdi
        pop rbx
        pop rdi
        pop rcx
        add rdi, 0x1000
        add rbx, 8
        dec rcx
        jmp .loop
    .exit1:
    pop rax
    test rax, rax
    jz .NoMoreSectors
    ; Setup packet
    mov [DiskLbaPacket.NumSectors + BOOT_PARTITION_BASE_ADDRESS], ax
    mov [DiskLbaPacket.Lba + BOOT_PARTITION_BASE_ADDRESS], rbx
    
    mov word [DiskLbaPacket.RealModeAddress + BOOT_PARTITION_BASE_ADDRESS], DiskTransferBuffer
    CallRealMode64 Int13, .R1
    
.R1:
    ; Copy content into edi
    mov rcx, rax ; 4 dword * 0x800 = 0x1000
    shl rcx, 9
    mov rsi, DiskTransferBuffer
    shr rcx, 3 ; Divide by 8
    rep movsq

    .NoMoreSectors:

    
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret

times 0x11000 - ($-$$) db 0
ErrorHandler:
    mov rax, 0xDEADCAFFE
    xor rdi, rdi
    mov di, StrCPUError
    call _print64
    jmp halt64
    


align 0x10
IDT64:
%rep 0x20
dw 0xA200
dw 0x20
db 0 ; IST0
db 0x8E
dw 0x1
dd 0
dd 0
%endrep
IDT64_END:

align 0x10
IDTR64:
    dw IDT64_END - IDT64 - 1
    dq IDT64 + BOOT_PARTITION_BASE_ADDRESS


; RBX = Cluster Start (Current Cluster), R15 = Buffer

; FUNCTION : R8 = Last AT Sector, R9 = Sector, R10 = Offset, RDX = FAT_LIKE_ENTRY
; R11 = DATA_CLUSTERS Start, R12 = Cluster size in sectors, r13 = Cluster Size in bytes
; DISK_READ : RCX = num sectors, RDI = buffer, RBX = LBA

