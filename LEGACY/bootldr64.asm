

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



; Relocate Kernel Image

mov rax, [KernelPeHdrAddress]
add rax, 176 ; Base Relocation Table

    
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

; Setup Frame Buffer Descriptor
    mov ax, [VbeModeInfo.Width + BOOT_PARTITION_BASE_ADDRESS]
    
    mov [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 4], ax ; Horizontal Resolution
    mov ax, [VbeModeInfo.Height + BOOT_PARTITION_BASE_ADDRESS]
    mov [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 8], ax ; Vertical Resolution
    mov dword [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 0xC], 0x10000 ; Version
    mov eax, [VbeModeInfo.FrameBufferAddress + BOOT_PARTITION_BASE_ADDRESS]
    mov ecx, [VbeModeInfo.OffsetScreenMemory + BOOT_PARTITION_BASE_ADDRESS]
    add rax, rcx
    mov [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 0x10], rax ; FrameBufferBase
    ; Frame Buffer Size

    xor rax, rax
    mov ax, [VbeModeInfo.Pitch + BOOT_PARTITION_BASE_ADDRESS]
    mov ecx, [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 8]
    mov [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 48], rax ; Pitch
    mul rcx
    mov [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 0x18], rax ; FrameBufferSize
    
    ; Color Depth, Refresh Rate
    mov ax, [VbeModeInfo.BitsPerPixel + BOOT_PARTITION_BASE_ADDRESS]
    mov [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 0x20], ax
    mov dword [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 0x24], 60 ; 60 FPS (Assumming)



; Setup InitData Structure

; Fill InitData with 0 (Already filled with Magic Number)

mov rdi, [InitData]
mov rcx, 0x20
xor rax, rax
rep stosq

mov rax, [InitData]
mov qword [rax], MemoryMap + BOOT_PARTITION_BASE_ADDRESS
mov qword [rax + 8], 1 ; NumFrameBuffer
mov qword [rax + 0x10], FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS
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
mov rbx, [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 0x10] ; FB Base
mov rcx, [FrameBufferDescriptor + BOOT_PARTITION_BASE_ADDRESS + 0x18] ; FB Size
shr rcx, 12 ; Divide by 0x1000
inc rcx ; Padding

call IdentityMapPages


wbinvd


jmp rax

.R0: ; Return Address 0
    cli
    hlt
    jmp .R0

StrInvalidKernelImage db "Invalid Kernel Image, please re-install your Operating System.", 13, 10, 0




align 0x10
FrameBufferDescriptor:
    times 0x100 db 0



StrFailedToLoadResources db "Failed to load resources. Please re-install your Operating System.", 13, 10, 0


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
    add [BootLoaderMemory], rcx
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
    mov rcx, 1
    jmp halt64

halt64:
    mov rax, 0xdeadcafe
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
    dw 0
    dq 0


; RBX = Cluster Start (Current Cluster), R15 = Buffer

; FUNCTION : R8 = Last AT Sector, R9 = Sector, R10 = Offset, RDX = FAT_LIKE_ENTRY
; R11 = DATA_CLUSTERS Start, R12 = Cluster size in sectors, r13 = Cluster Size in bytes
; DISK_READ : RCX = num sectors, RDI = buffer, RBX = LBA

