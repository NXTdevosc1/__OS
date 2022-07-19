nasm src/memopt.asm -f win64 -o obj/x86_64/Assembly/memopt.obj
nasm src/system.asm -f win64 -o obj/x86_64/Assembly/system.obj


cl src/*.c obj/x86_64/Assembly/*.obj /GS- /Fe:osdll.dll /Fo:obj/x86_64/ /LD /Od /Iinc /I../inc /I../drv/inc /link /DYNAMICBASE /FIXED:no /MACHINE:x64 /subsystem:native /dll /nodefaultlib
copy /b osdll.dll "../../kernel/iso/os/system"
copy /b osdll.lib "../../drivers/"
copy /b osdll.lib "../../applications"
