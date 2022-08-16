rd /s /q x86_64
mkdir x86_64\
mkdir x86_64\Assembly
mkdir x86_64\fs\fat32
FOR /R "src" %%S IN (*.asm) DO (
	nasm %%S -f win64 -o x86_64/Assembly/%%~nS.s.obj

)

set srcfiles=src/*.c src/smbios/*.c src/acpi/*.c src/acpi/aml/*.c src/CPU/*.c src/dsk/*.c src/input/*.c src/interrupt_manager/*.c src/IO/*.c src/ipc/*.c src/lib/*.c src/loaders/*.c src/Management/*.c src/sys/*.c src/sysentry/*.c src/typography/*.c src/utils/*.c src/timedate/*.c
set COMPILE=cl /DEFAULTLIB:no /nologo /KERNEL 
set CFLAGS=/GS- /Ilib /I../libc/drv/inc /I../UEFI/gnu-efi/inc /I../UEFI/gnu-efi/inc/x86_64 /Iinc /Ilib

set OUT=/Fo:x86_64

set OBJFILES=x86_64/Assembly/*.obj x86_64/fs/*.obj x86_64/fs/fat32/*.obj


%COMPILE% /c src/fs/*.c %OUT%/fs/ %CFLAGS%
%COMPILE% /c src/fs/fat32/*.c %OUT%/fs/fat32/ %CFLAGS%

%COMPILE% %srcfiles% %OBJFILES% /Fo:x86_64/ %CFLAGS% /Fe:oskrnlx64.exe /LD /link /DLL /MACHINE:x64 /NODEFAULTLIB /SUBSYSTEM:native /ENTRY:KrnlEntry /FIXED:no /DYNAMICBASE /LARGEADDRESSAWARE

@REM wsl cd ../UEFI/gnu-efi; make bootloader

copy oskrnlx64.exe iso\OS\System
copy "..\UEFI\edk2\Build\MdeModule\DEBUG_GCC5\X64\EfiBoot\bootx64\OUTPUT\EfiBoot.efi" "iso\efi\boot\bootx64.efi"
copy oskrnlx64.lib "..\drivers"
@REM wsl make build_iso
@REM wsl make img

@REM cd ../LEGACY

@REM "imgsetupnoadmin" fstest

@REM copy os.img "..\kernel\bin"

@REM cd ../kernel