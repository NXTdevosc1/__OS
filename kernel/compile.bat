@REM rd /s /q x86_64
mkdir x86_64\
mkdir x86_64\Assembly
mkdir x86_64\fs\fat32
FOR /R "src" %%S IN (*.asm) DO (
	nasm "%%S" -Ox -f win64 -o "x86_64/Assembly/%%~nS.s.obj"

)

set srcfiles=src/fs/*.c src/*.c "src/Memory Management/*.c" src/smbios/*.c src/acpi/*.c src/acpi/aml/*.c src/CPU/*.c src/dsk/*.c src/input/*.c src/interrupt_manager/*.c src/IO/*.c src/ipc/*.c src/lib/*.c src/loaders/*.c src/Management/*.c src/sys/*.c src/sysentry/*.c src/typography/*.c src/utils/*.c src/timedate/*.c
set COMPILE=cl /DEFAULTLIB:no
set CFLAGS= /O2 /Gr /GS- /wd4005 /wd4711 /wd4710 /wd4213 /Wall /wd4820 /wd4200 /wd4152 /wd4100 /wd5045 /wd4189 /wd4702 /wd4255 /Ilib /I../libc/drv/inc /I../UEFI/edk2/MdePkg/Include /I../UEFI/edk2/MdePkg/Include/X64 /Iinc /Ilib
@REM Remove /wd4710 (function not inlined)
set OUT=/Fo:x86_64

set OBJFILES=x86_64/Assembly/*.obj


@REM %COMPILE% /c %OUT%/fs/ %CFLAGS%
@REM %COMPILE% /c src/fs/fat32/*.c %OUT%/fs/fat32/ %CFLAGS%

%COMPILE% %srcfiles% %OBJFILES% /Fo:x86_64/ %CFLAGS% /DEBUG:no /Fe:oskrnlx64.exe /LD /link /OPT:LBR,REF /DLL /MACHINE:x64 /NODEFAULTLIB /SUBSYSTEM:native /ENTRY:KrnlEntry /FIXED:no /DYNAMICBASE /LARGEADDRESSAWARE

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