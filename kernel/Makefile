__IMGNAME = os

GNUEFI = ../UEFI/gnu-efi
OVMFDIR = ../OVMFbin
LDS = kernel.ld
CC = gcc
ASMC = nasm
LD = ld
# Make GCC Do not use syscall registers R10, R13, R14
CFLAGS = -O0 -nostdlib -ffreestanding -ffixed-r10 -ffixed-r13 -ffixed-r14 -fshort-wchar -mno-red-zone -c -I \
inc -I lib -I ../UEFI/gnu-efi/inc -I ../UEFI/gnu-efi/inc/x86_64 -I ../libc/drv/inc

ASMFLAGS = -O0
LDFLAGS = -T $(LDS) -Bsymbolic -nostdlib

SRCDIR := src
OBJDIR := x86_64
BUILDDIR = bin
BOOTEFI := $(GNUEFI)/x86_64/bootloader/main.efi

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC = $(call rwildcard,$(SRCDIR),*.c)
ASMSRC = $(call rwildcard,$(SRCDIR),*.asm)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))
OBJS += $(patsubst $(SRCDIR)/%.asm, $(OBJDIR)/%_asm.o, $(ASMSRC))

DISKDIR = iso

IMG = $(call rwildcard, $(DISKDIR), *)

KNAME=OSKRNLX64.exe
OSNAME=OS
KPATH=$(OSNAME)/System/$(KNAME)
DIRS = $(wildcard $(SRCDIR)/*)

all:
	@ cd ../UEFI/gnu-efi; make bootloader
	# @ make kernel
	@ make build_iso
	@ make img

build_iso:
	mkdir -p iso/$(OSNAME)
	mkdir -p iso/$(OSNAME)/System
	mkdir -p iso/$(OSNAME)/Fonts
	mkdir -p iso/$(OSNAME)/Icons
	cp $(KNAME) iso/$(KPATH)
	cp bin/zap-light16.psf iso/$(OSNAME)/Fonts/zap-light16.psf
	cp ../UEFI/edk2/Build/MdeModule/DEBUG_GCC5/X64/EfiBoot/bootx64/OUTPUT/EfiBoot.efi iso/efi/boot/bootx64.efi
	genisoimage -o /mnt/c/users/loukd/desktop/test.iso -R -J iso/
	
kernel: $(OBJS) link

$(OBJDIR)/interrupt_manager/interrupts.o: $(SRCDIR)/interrupt_manager/interrupts.c
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -mgeneral-regs-only -c $^ -o $@


$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@


$(OBJDIR)/%_asm.o: $(SRCDIR)/%.asm
	@ echo !==== COMPILING $^
	@ mkdir -p $(@D)
	$(ASMC) $(ASMFLAGS) $^ -f elf64 -o $@

link:
	@ echo !==== LINKING
	$(LD) $(LDFLAGS) -o $(KNAME) $(OBJS)
	objcopy -O pei-x86-64 --subsystem=1:1.0 --file-alignment=0x1000 --section-alignment=0x1000 OSKRNLX64.exe OSKRNLX64.exe

setup:
	@mkdir $(BUILDDIR)
	@mkdir $(SRCDIR)
	@mkdir $(OBJDIR)



img:

	mformat -i $(BUILDDIR)/$(__IMGNAME).img -v "Long Name In Dev OS Drive" -F ::


	mmd -i $(BUILDDIR)/$(OSNAME).img ::/$(OSNAME)
	mmd -i $(BUILDDIR)/$(OSNAME).img ::/$(OSNAME)/System
	mmd -i $(BUILDDIR)/$(OSNAME).img ::$(OSNAME)/Fonts
	#mmd -i $(BUILDDIR)/$(OSNAME).img ::$(OSNAME)/Apps



	mcopy -i $(BUILDDIR)/$(OSNAME).img iso/* ::
	mcopy -i $(BUILDDIR)/$(OSNAME).img iso/efi/* ::/efi
	#mcopy -i $(BUILDDIR)/$(OSNAME).img startup.nsh ::
	mcopy -i $(BUILDDIR)/$(OSNAME).img iso/$(OSNAME)/System/* ::/$(OSNAME)/System/
	mcopy -i $(BUILDDIR)/$(OSNAME).img iso/$(OSNAME)/Fonts/* ::/$(OSNAME)/Fonts/

	#mcopy -i $(BUILDDIR)/$(OSNAME).img iso/$(OSNAME)/Icons/* ::
	#mcopy -i $(BUILDDIR)/$(OSNAME).img iso/$(OSNAME)/System/Modules/* ::
	#mcopy -i$(BUILDDIR)/$(OSNAME).img iso/$(OSNAME)/System/Apps/* ::/$(OSNAME)/System/Apps/
	



	


run:
	qemu-system-x86_64 -drive file=$(BUILDDIR)/$(__IMGNAME).img -m 256M -cpu qemu64 -drive if=pflash,format=raw,unit=0,file="$(OVMFDIR)/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="$(OVMFDIR)/OVMF_VARS-pure-efi.fd" -net none

reserved:
	dd if=/dev/zero of=$(BUILDDIR)/$(__IMGNAME).img bs=512 count=90750

