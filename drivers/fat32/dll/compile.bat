cl src/*.c /Od /Fo:obj/x86_64/ /Iinc /GS- /LD /Fe:fat32.dll /link /MACHINE:x64 /dll /nodefaultlib /FIXED:no /DYNAMICBASE /Entry:DllMain