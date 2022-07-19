set SRCFILES=src/*.c src/xhci/*.c src/ehci/*.c src/ohci/*.c src/uhci/*.c
cl /Od %SRCFILES% obj/x86_64/Assembly/*.obj /GS-  "../gdk.lib" "../kernelruntime.lib" "../osdll.lib" /KERNEL /Fo:obj/x86_64/ /I inc /I ../../libc/inc /I ../../libc/drv/inc  /Fe:usb.sys /link /DYNAMICBASE /nodefaultlib /manifest:no /FIXED:NO /subsystem:native /machine:x64 /entry:DriverEntry /STACK:0x1000000
copy /b usb.sys "../../kernel/iso/os/system"
