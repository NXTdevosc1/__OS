wsl make
copy bootx64.efi "../../kernel/iso/efi/boot/"
cd ..
imgsetupnoadmin.exe fstest
cd NVMe_EFI_Bootloader
qemu-system-x86_64 -m 4G  -monitor stdio -drive file=../os.img,format=raw -drive file=../os.img,format=raw,if=none,id=nvm -device nvme,drive=nvm,serial=cafe -drive if=pflash,format=raw,unit=0,file=OVMF.fd,readonly=on