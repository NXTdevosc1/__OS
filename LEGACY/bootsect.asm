[ORG 0x7C00]
[BITS 16]

; FAT32 Format Bootsector


; ; JMP

; _jmp:
; 	jmp $ + BOOT_OFFSET

; ; OEM NAME
; db "_DEVOS__"
; ; BIOS_PARAMETER_BLOCK
; dw 0x200
; db 1
; dw 0x100
; db 2
; dw 0
; dw 0
; db 0xF8 ; Hard disk
; dw 0

; ; DOS3_PARAMETER_BLOCK
; dw 0x3F ; All modern drives use 63 sectors per track
; dw 0
; dd 0
; dd 1

; ; EXTENDED_BIOS_PARAMETER_BLOCK

; dd 1
; dw 0
; dw 0x100
; dd 2
; dw 0
; dw 0
; times 12 db 0 ; Reserved
; db 0 ; PHYSICAL_DRIVE_NUMBER
; db 0
; db 0x29 ; EXTENDED_BOOT_SIGNATURE
; dd 0 ; VOLUME_ID
; db "HYBRID_BOOT"
; db "FAT32",0,0,0


MBR_PARTITION_ADDRESS equ 0x7DCE

jmp _boot
BootDrive db 0

BOOT_PARTITION_BASE_ADDRESS equ 0x9000

BOOT_MANAGER_OFFSET equ 0x200

BOOT_MANAGER_MAGIC1 equ 0xFB7E

BOOT_AREA_SIZE equ 0x100 ; 256 Sectors

_boot:
	cld
	cli
	; Reset segment registers
	xor ax, ax
	mov es, ax
	mov ds, ax
	mov ss, ax

	mov sp, 0x8000
	mov bp, sp

	mov bx, BootDrive

	mov [es:bx], dl



	; Check if partition is active
	mov al, [es:MBR_PARTITION_ADDRESS]
	test al, 0x80
	jz ErrInvalidOrCorruptedFs

	mov bx, MBR_PARTITION_ADDRESS
	; mov ch, [es:bx + 3]
	mov eax, [es:bx + 8] ; Lba Start
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

	mov bx, [BOOT_PARTITION_BASE_ADDRESS + BOOT_MANAGER_OFFSET + 0x10] ; 0x10 offset of boot entry pointer
	mov dl, [BootDrive] ; Send boot drive to BOOT_MANAGER
	jmp bx

READ_PART2_SEG equ 0x1120
READ_PART3_SEG equ 0x1920
READ_PART4_SEG equ 0x2120
LbaPacket:
	db 0x10
	db 0
	.NumSectors dw 0x41 ; + 1 (PARTITION BOOTSECTOR) (read in 4 parts)
	.Address dw 0
	.Segment dw 0x900 ; Address 0x9000 (BOOT_PARTITION_BASE_ADDRESS)
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
	int 0x13
	jc .err
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
		mov al, [es:di]
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

BOOTMGR_MAGIC0 db "BOOTMGR_"
InvalidOrCorruptedFs db "Invalid or corrupted file system", 13, 10, "Halting...", 13, 10, 0
CouldNotReadHardDisk db "Could not read from hard disk.", 13, 10, "Halting...", 13, 10, 0
BOOT_CODE_LENGTH equ 440

times BOOT_CODE_LENGTH - ($-$$) db 0