
@REM "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

@REM .\compile
@REM .\imgsetup devctl D:



nasm -O0 bootsect.asm -f bin -o x86_64/bootsect.bin
nasm -O0 bootmgr.asm -f bin -o x86_64/osbootmgr.bin
copy "..\drivers\fat32\dll\fat32.dll" fat32.dll
imgsetupnoadmin fstest

qemu-system-x86_64 -cpu max,+kvm,hypervisor=on,hv-passthrough,hv_time,hv-vapic,hv-runtime,hv-synic,hv-tlbflush,hv-stimer,hv-ipi,hv-reenlightenment,hv-evmcs,hv-avic,hv-stimer-direct,hv-enforce-cpuid -m 4G -smp 4,cores=4,threads=1 -monitor stdio -drive file=os.img,format=raw -device qemu-xhci -device usb-kbd