
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_M),aarch64)
PREFIX:=i686-linux-gnu-
BOOTIMG:=/usr/local/grub/lib/grub/i386-pc/boot.img
GRUBLOC:=/usr/local/grub/bin/
else
PREFIX:=
BOOTIMG:=/usr/lib/grub/i386-pc/boot.img
GRUBLOC :=
endif

CC := $(PREFIX)i686-elf-gcc
LD := $(PREFIX)i686-elf-ld
OBJDUMP := $(PREFIX)objdump
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
CONFIGS := -DCONFIG_HEAP_SIZE=4096
CFLAGS := -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall 

ODIR = obj
SDIR = src

OBJS = \
	kernel_main.o \

# Make sure to keep a blank line here after OBJS list

OBJ = $(patsubst %,$(ODIR)/%,$(OBJS))

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) -c -g -o $@ $^

$(ODIR)/%.o: $(SDIR)/%.s
	$(CC) $(CFLAGS) -c -g -o $@ $^


all: bin rootfs.img

bin: obj $(OBJ)
	$(LD) -melf_i386  obj/* -Tkernel.ld -o kernel
	# $(SIZE) kernel
	i686-elf-size kernel
	i686-elf-size kernel

obj:
	mkdir -p obj

rootfs.img:
	dd if=/dev/zero of=rootfs.img bs=1M count=32
	$(GRUBLOC)grub-mkimage -p "(hd0,msdos1)/boot" -o grub.img -O i386-pc normal biosdisk multiboot multiboot2 configfile fat exfat part_msdos
	dd if=/usr/local/grub/lib/grub/i386-pc/boot.img  of=rootfs.img conv=notrunc
	dd if=$(BOOTIMG) of=rootfs.img conv=notrunc
	echo 'start=2048, type=83, bootable' | sfdisk rootfs.img
	mkfs.vfat --offset 2048 -F16 rootfs.img
	mcopy -i rootfs.img@@1M kernel ::/
	mmd -i rootfs.img@@1M boot 
	mcopy -i rootfs.img@@1M grub.cfg ::/boot
	@echo " -- BUILD COMPLETED SUCCESSFULLY --"


run:
	qemu-system-i386 -hda rootfs.img

debug:
	./launch_qemu.sh

clean:
	rm -f grub.img kernel rootfs.img obj/*
