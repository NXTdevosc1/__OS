
@REM "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

@REM .\compile
@REM .\imgsetup devctl D:



nasm -O0 bootsect.asm -f bin -o x86_64/bootsect.bin
nasm -O0 bootmgr.asm -f bin -o x86_64/osbootmgr.bin
nasm -O0 partbootsect.asm -f bin -o x86_64/partbootsect.bin

copy "..\drivers\fat32\dll\fat32.dll" fat32.dll
imgsetupnoadmin fstest

qemu-system-x86_64 -accel hax -no-shutdown -no-reboot -cpu max,xsave=on,avx=on,vmx=off,avx2=on,avx512f=on,avx512dq=on -m 2G -smp 1,cores=1,threads=1 -machine q35 -monitor stdio -drive file=os.img,format=raw -device qemu-xhci -device usb-kbd