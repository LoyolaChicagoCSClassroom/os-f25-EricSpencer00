
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_M),aarch64)
PREFIX:=i686-linux-gnu-
BOOTIMG:=/usr/local/grub/lib/grub/i386-pc/boot.img
GRUBLOC:=/usr/local/grub/bin/
else
PREFIX:=
# On macOS with Homebrew, the i686-elf-grub formula installs boot.img under its Cellar
ifneq (,$(shell test -f /usr/local/Cellar/i686-elf-grub/2.12/lib/i686-elf/grub/i386-pc/boot.img && echo yes))
	BOOTIMG:=/usr/local/Cellar/i686-elf-grub/2.12/lib/i686-elf/grub/i386-pc/boot.img
else
	BOOTIMG:=/usr/lib/grub/i386-pc/boot.img
endif
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

# Fallback: build a bootable ISO with GRUB (useful on macOS where sfdisk/mkfs may be missing)
ISO_DIR := iso_tmp
ISO := grub.iso

.PHONY: iso run_iso
iso: bin $(ISO)


$(ISO): kernel grub.cfg
	rm -f $(ISO)
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	# Put kernel at the ISO root so grub.cfg referencing /kernel works
	cp kernel $(ISO_DIR)/
	cp kernel $(ISO_DIR)/boot/
	cp grub.cfg $(ISO_DIR)/boot/grub/
	if command -v i686-elf-grub-mkrescue >/dev/null 2>&1; then \
		 i686-elf-grub-mkrescue -o $(ISO) $(ISO_DIR); \
	else \
		 grub-mkrescue -o $(ISO) $(ISO_DIR); \
	fi
	# verify grub-mkrescue actually created the ISO
	if [ ! -f $(ISO) ]; then \
		echo "ERROR: grub-mkrescue failed to create $(ISO)" >&2; \
		exit 1; \
	fi

run_iso: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -serial stdio


bin: obj $(OBJ)
	$(LD) -melf_i386  obj/* -Tkernel.ld -o kernel
	# $(SIZE) kernel
	i686-elf-size kernel
	i686-elf-size kernel

obj:
	mkdir -p obj

rootfs.img:
	dd if=/dev/zero of=rootfs.img bs=1M count=32
	# Use the prefixed grub-mkimage provided by the i686-elf-grub formula on macOS
	if command -v i686-elf-grub-mkimage >/dev/null 2>&1; then \
		 i686-elf-grub-mkimage -p "(hd0,msdos1)/boot" -o grub.img -O i386-pc normal biosdisk multiboot multiboot2 configfile fat exfat part_msdos; \
	else \
		 $(GRUBLOC)grub-mkimage -p "(hd0,msdos1)/boot" -o grub.img -O i386-pc normal biosdisk multiboot multiboot2 configfile fat exfat part_msdos; \
	fi
	dd if=$(BOOTIMG) of=rootfs.img conv=notrunc
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
