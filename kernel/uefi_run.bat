cd ../LEGACY
"imgsetupnoadmin" fstest
cd ../kernel
qemu-system-x86_64 -cpu max -smp 4,cores=4 -device usb-ehci -device usb-kbd -device usb-mouse -m 4G -vga vmware -no-reboot -no-shutdown  -monitor stdio -drive file=../LEGACY/os.img,format=raw -drive if=pflash,format=raw,unit=0,file=OVMF.fd,readonly=on