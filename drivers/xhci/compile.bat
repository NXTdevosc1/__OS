set SRCFILES=src/*.c
cl %SRCFILES% /GS-  "../ddk.lib" "../oskrnlx64.lib" "../osdll.lib" /KERNEL /Fo:obj/x86_64/ /I inc /I ../../libc/inc /I ../../libc/drv/inc  /Fe:xhci.sys /link /DYNAMICBASE /nodefaultlib /manifest:no /FIXED:NO /subsystem:native /machine:x64 /entry:DriverEntry
copy /b xhci.sys "../../kernel/iso/os/system"
