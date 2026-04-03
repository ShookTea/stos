# Compiler / tools
CC := i686-elf-gcc
AS := i686-elf-as
AR := i686-elf-ar

BUILD_DIR      := build
KERNEL_SRC_DIR := kernel
LIBC_DIR       := libc
TARGET         := $(BUILD_DIR)/stos
INITRD         := $(BUILD_DIR)/isodir/boot/stos.initrd

QEMU_FLAGS := -m 512M -serial stdio -boot order=dc

include libc/Rules.mk
include kernel/Rules.mk
include usermode/Rules.mk

.PHONY: all clean qemu

all: $(TARGET).iso

qemu: $(TARGET).iso
	# Boot order "dc": boot first from cdrom (d), then from hard disk (c)
	qemu-system-i386 -cdrom $^ $(QEMU_FLAGS)

$(TARGET).iso: $(TARGET) kernel/grub.cfg $(INITRD)
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(TARGET) $(BUILD_DIR)/isodir/boot/stos
	cp kernel/grub.cfg $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/isodir

clean:
	rm -rf $(BUILD_DIR)
