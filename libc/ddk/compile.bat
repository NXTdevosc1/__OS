nasm src/ddk.asm -f win64 -o obj/x86_64/Assembly/gdk.obj
cl src/*.c "../kernelruntime/kernelruntime.lib" "../osdll/osdll.lib" /GS- obj/x86_64/Assembly/*.obj /Fe:ddk.dll /Fo:obj/x86_64/ /Od /I../../kernel/inc /I../inc /I../drv/inc /LD  /link /DYNAMICBASE /FIXED:no /subsystem:native /dll /nodefaultlib
copy /b ddk.dll "../../kernel/iso/os/system"
copy /b ddk.lib "../../drivers/"
