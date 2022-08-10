[ORG 0x600]
[BITS 16]

BOOT_BASE equ 0x600


MBR_PARTITION_ADDRESS equ BOOT_BASE + 0x1CE

	cli
	cld
	or dl, 0x80
	xor ax, ax
	mov es, ax
	mov ds, ax
	mov ss, ax
	mov gs, ax
	mov fs, ax

	; Relocate bootsector to 0:0x600
	mov si, 0x7C00
	mov di, 0x600
	mov cx, 0x80 ; 0x80 * 4 = 512
	rep movsd
	jmp 0:_boot

BootDrive db 0

BOOT_PARTITION_BASE_ADDRESS equ 0x7C00

BOOT_MANAGER_OFFSET equ 0x200

BOOT_MANAGER_MAGIC1 equ 0xFB7E

BOOT_AREA_SIZE equ 0x100 ; 256 Sectors



_boot:
	; Reset segment registers
	

	mov sp, 0x1000
	mov bp, sp

	mov bx, BootDrive

	mov [bx], dl

	


	; Check if partition is active
	mov al, [MBR_PARTITION_ADDRESS]
	test al, 0x80
	jz ErrInvalidOrCorruptedFs

	mov bx, MBR_PARTITION_ADDRESS
	; mov ch, [es:bx + 3]
	mov eax, [bx + 8] ; Lba Start
	mov [LbaPacket.Lba], eax

	call LbaRead


	; Check BOOT_MANAGER Header
	mov bx, BOOT_PARTITION_BASE_ADDRESS + BOOT_MANAGER_OFFSET
	
	; CMP MAGIC0
	.magic0:
		mov di, BOOTMGR_MAGIC0
		mov al, 8
		.loop:
			test al, al
			jz .exit
			mov cl, [di]
			mov ch, [bx]
			cmp cl, ch
			jne ErrInvalidOrCorruptedFs
			inc di
			inc bx
			dec al
			jmp .loop
		.exit:

	; CMP MAGIC1
	.magic1:
		mov bx, BOOT_PARTITION_BASE_ADDRESS + BOOT_MANAGER_OFFSET + 8
		cmp word [bx], BOOT_MANAGER_MAGIC1
		jne ErrInvalidOrCorruptedFs

	; mov bx, [BOOT_PARTITION_BASE_ADDRESS + BOOT_MANAGER_OFFSET + 0x10] ; 0x10 offset of boot entry pointer
	mov dl, [BootDrive] ; Send boot drive to BOOT_MANAGER

	jmp 0x7C00

READ_PART2_SEG equ 0xFE0
READ_PART3_SEG equ 0x17E0
READ_PART4_SEG equ 0x1FE0
LbaPacket:
	db 0x10
	db 0
	.NumSectors dw 0x41 ; + 1 (PARTITION BOOTSECTOR) (read in 4 parts)
	.Address dw 0
	.Segment dw BOOT_PARTITION_BASE_ADDRESS / 0x10 ; Address 0x7C00 (BOOT_PARTITION_BASE_ADDRESS)
	.Lba dq 0 ; Set by bootsector


LbaRead:
	; Part 1
	mov si, LbaPacket
	mov ah, 0x42
	mov dl, [BootDrive]
	int 0x13
	jc .err
	; Part 2
	add dword [LbaPacket.Lba], 0x41
	mov dword [LbaPacket.NumSectors], 0x40
	mov word [LbaPacket.Segment], READ_PART2_SEG

	mov si, LbaPacket
	mov ah, 0x42
	mov dl, [BootDrive]
	int 0x13
	jc .err

	; Parts 3 - 8
	mov cx, 5 ; Count
	mov bx, READ_PART3_SEG ; Segment (inc by 0x800)
.loop:
	test cx, cx
	jz .exit
	; Part 3
	add dword [LbaPacket.Lba], 0x40
	mov dword [LbaPacket.NumSectors], 0x40
	mov word [LbaPacket.Segment], bx

	mov si, LbaPacket
	mov ah, 0x42
	mov dl, [BootDrive]
	push cx
	int 0x13
	jc .err
	pop cx
	add bx, 0x800 ; Segment inc
	dec cx
	jmp .loop
.exit:
	ret
.err:
	mov di, CouldNotReadHardDisk
	call _print
	jmp _halt

_print:
	xor bx, bx
	mov ah, 0x0E
	.loop:
		mov al, [di]
		test al, al
		jz .exit
		inc di
		int 0x10
		jmp .loop
	.exit:
		ret	

_halt:
	cli
	hlt
	jmp _halt


ErrInvalidOrCorruptedFs:
	mov di, InvalidOrCorruptedFs
	call _print
	jmp _halt

BOOTMGR_MAGIC0 db "BOOTMGR_",0
InvalidOrCorruptedFs db "Invalid or corrupted file system", 13, 10, "Halting...", 13, 10, 0
CouldNotReadHardDisk db "Could not read from hard disk.", 13, 10, "Halting...", 13, 10, 0
BOOT_CODE_LENGTH equ 440

times BOOT_CODE_LENGTH - ($-$$) db 0