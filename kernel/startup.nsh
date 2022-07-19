@echo -off
mode 80 25

cls
if exist .\efi\boot\bootx64.efi then
 .\efi\boot\bootx64.efi
 goto END
endif

if exist fs0:\efi\boot\bootx64.efi then
 fs0:
 efi\boot\bootx64.efi
 goto END
endif

echo "UNABLE TO FIND BOOTLOADER".

:END
