qemu-system-x86_64 -cpu Skylake-Server-v2,+kvm,hypervisor=on,hv-passthrough,hv_time,hv-vapic,hv-runtime,hv-synic,hv-tlbflush,hv-stimer,hv-ipi,hv-reenlightenment,hv-evmcs,hv-avic,hv-stimer-direct,hv-enforce-cpuid -machine q35 -device usb-ehci -device usb-kbd -device usb-mouse -smp 16,sockets=2,cores=4,threads=2 -cpu max -m 4G -vga vmware -no-reboot -no-shutdown  -monitor stdio -drive file=../LEGACY/os.img,format=raw -drive if=pflash,format=raw,unit=0,file=OVMF_CODE.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd -drive file=../LEGACY/os.img,format=raw