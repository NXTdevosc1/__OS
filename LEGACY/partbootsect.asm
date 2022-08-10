
; Partition Boot Loader

[BITS 16]
[ORG 0x7C00]

; Jmp
jmp _Start

times 89 db 0
_Start:
    jmp 0:.Load
.Load:
    jmp [0x7E10] ; Contains the address of the bootmanager



_Print:
    mov ah, 0xe
    mov al, [di]
    test al, al
    jz .Ex
    int 10h
    inc di
    jmp _Print
.Ex:
    ret

HelloWorld db "Hello World", 0

times 510 - ($-$$) db 0
dw 0xAA55