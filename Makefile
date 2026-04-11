# Compiler / tools
CC := i686-elf-gcc
AS := i686-elf-as
AR := i686-elf-ar

BUILD_DIR      := build
KERNEL_SRC_DIR := kernel
LIBC_DIR       := libc
LIBDS_DIR      := libds
TARGET         := $(BUILD_DIR)/stos
INITRD         := $(BUILD_DIR)/isodir/boot/stos.initrd
HARD_DRIVE     := .qemu/disk.img

LIB_INCLUDE_FLAGS := -I$(LIBC_DIR)/include -I$(LIBDS_DIR)/include

# Boot order:
# "c" - hard disk
# "d" - cdrom
QEMU_FLAGS := -m 512M -serial stdio -boot order=dc

include libc/Rules.mk
include libds/Rules.mk
include kernel/Rules.mk
include usermode/Rules.mk

.PHONY: all clean qemu test

all: $(TARGET).iso

qemu: $(TARGET).iso $(HARD_DRIVE)
	qemu-system-i386 \
	    -cdrom $(TARGET).iso \
		-drive file=$(HARD_DRIVE),format=raw,media=disk \
		$(QEMU_FLAGS)

$(TARGET).iso: $(TARGET) kernel/grub.cfg $(INITRD)
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(TARGET) $(BUILD_DIR)/isodir/boot/stos
	cp kernel/grub.cfg $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/isodir

test: $(LIBDS_TEST_TARGET)
	$(LIBDS_TEST_TARGET)

$(HARD_DRIVE):
	@mkdir -p $(@D)
	qemu-img create $@ 32G
	echo "create"
	touch $@

clean-all: clean
	rm -rf .qemu

clean:
	rm -rf $(BUILD_DIR)
