cl src/*.c /Fo:obj/x86_64/ /Od /I../inc /I../drv/inc /LD /link /DYNAMICBASE /FIXED:no /Fe:kernelruntime.dll /subsystem:native /dll /nodefaultlib
copy /b kernelruntime.dll "../../kernel/iso/os/system"
copy /b kernelruntime.lib "../../drivers/"