set SRCFILES=src/*.c
cl %SRCFILES% /Od /GS-  "../ddk.lib" "../oskrnlx64.lib" "../osdll.lib" /KERNEL /Fo:obj/x86_64/ /I inc /I ../../libc/inc /I ../../libc/drv/inc  /Fe:ahci.sys /link /DYNAMICBASE /nodefaultlib /manifest:no /FIXED:NO /subsystem:native /machine:x64 /entry:DriverEntry
copy /b ahci.sys "../../kernel/iso/os/system"
