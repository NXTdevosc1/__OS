
cl src/*.c obj/x86_64/Assembly/*.obj  "../gdk.lib" "../kernelruntime.lib" "../osdll.lib" /KERNEL /Fo:obj/x86_64/ /I inc /I ../../libc/inc /I ../../libc/drv/inc  /Fe:osdwm.sys /link /DYNAMICBASE /nodefaultlib /manifest:no /FIXED:NO /subsystem:native /machine:x64 /entry:DriverEntry
copy /b osdwm.sys "../../kernel/iso/os/system"
