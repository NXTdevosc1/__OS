cl imgsetup.c /Od /Fo:obj/ /Fe:imgsetup.exe /MT "../drivers/fat32/dll/fat32.lib" /I../drivers/fat32/dll/inc /link /DYNAMICBASE /FIXED:no /MACHINE:x64 /SUBSYSTEM:console
copy imgsetup.exe imgsetupnoadmin.exe
copy "..\drivers\fat32\dll\fat32.dll" fat32.dll