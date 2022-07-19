set SRCFILES=src/*.c src/vmsvga/*.c src/intel/mobile4exp/*.c
cl %SRCFILES% obj/x86_64/Assembly/*.obj  "../gdk.lib" "../kernelruntime.lib" "../osdll.lib" /KERNEL /Fo:obj/x86_64/ /I inc /I ../../libc/inc /I ../../libc/drv/inc  /Fe:basicdisplay.sys /link /DYNAMICBASE /nodefaultlib /manifest:no /FIXED:NO /subsystem:native /machine:x64 /entry:DriverEntry
copy /b basicdisplay.sys "../../kernel/iso/os/system"
