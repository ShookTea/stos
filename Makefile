# Directories and paths
KERNEL_SRC_DIR := kernel
LIBC_SRC_DIR := libc
USERMODE_SRC_DIR := usermode
BUILD_DIR := build
LIBK_BUILD_DIR := $(BUILD_DIR)/lib
USERMODE_BUILD_DIR := $(BUILD_DIR)/usermode

# Compiler / tools
CC      := i686-elf-gcc
AS      := i686-elf-as
AR      := i686-elf-ar
CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra
CFLAGS  := $(CFLAGS) -I$(LIBC_SRC_DIR)/include
CFLAGS  := $(CFLAGS) -I$(KERNEL_SRC_DIR)/include
ASFLAGS := # asm flags if needed
LDFLAGS := -T kernel/arch/i386/linker.ld -ffreestanding -O2 -nostdlib -lgcc -z max-page-size=0x1000
LIBK_CFLAGS := $(CFLAGS) -D__is_libk
USERMODE_CFLAGS := -std=gnu99 -O2 -Wall -Wextra -I$(LIBC_SRC_DIR)/include -ffreestanding
USERMODE_LDFLAGS := -T $(USERMODE_SRC_DIR)/linker.ld -nostdlib -ffreestanding -lgcc

TARGET := build/stos
LIBK_TARGET := build/lib/libk.a
INITRD := build/isodir/boot/stos.initrd

QEMU_FLAGS := -m 512M -serial stdio -boot order=dc

# Kernel sources (exluding libc/libk)
KERNEL_C := $(shell find $(KERNEL_SRC_DIR)/kernel $(KERNEL_SRC_DIR)/arch -name '*.c')
KERNEL_S := $(shell find $(KERNEL_SRC_DIR)/kernel $(KERNEL_SRC_DIR)/arch -name '*.s')
KERNEL_SRCS := $(KERNEL_C) $(KERNEL_S)

# Libk sources
LIBK_C := $(shell find $(LIBC_SRC_DIR) -name '*.c')
LIBK_S := $(shell find $(LIBC_SRC_DIR) -name '*.s')
LIBK_SRCS := $(LIBK_C) $(LIBK_S)

# Usermode files - each file is treated as its own program
USERMODE_C := $(shell find $(USERMODE_SRC_DIR) -name '*.c' ! -name 'crt0.c')
USERMODE_SRCS := $(USERMODE_C)

# CRT0 is linked with all usermode programs
USERMODE_CRT0 := $(USERMODE_SRC_DIR)/crt0.c
USERMODE_CRT0_OBJ := $(USERMODE_BUILD_DIR)/crt0.u.o

# Convert e.g. kernel/foo/bar.c -> build/foo/bar.o
KERNEL_OBJS := $(patsubst $(KERNEL_SRC_DIR)/%,$(BUILD_DIR)/%,$(KERNEL_SRCS:.c=.o))
KERNEL_OBJS := $(patsubst $(KERNEL_SRC_DIR)/%,$(BUILD_DIR)/%,$(KERNEL_OBJS:.s=.o))
LIBK_OBJS := $(patsubst $(LIBC_SRC_DIR)/%,$(LIBK_BUILD_DIR)/%,$(LIBK_SRCS:.c=.libk.o))
LIBK_OBJS := $(patsubst $(LIBC_SRC_DIR)/%,$(LIBK_BUILD_DIR)/%,$(LIBK_OBJS:.s=.libk.o))
USERMODE_OBJS := $(patsubst $(USERMODE_SRC_DIR)/%,$(USERMODE_BUILD_DIR)/%,$(USERMODE_SRCS:.c=.u.o))

# Create usermode bin files (final ELF files after linking)
USERMODE_BIN := $(patsubst $(USERMODE_BUILD_DIR)/%.u.o,$(USERMODE_BUILD_DIR)/%,$(USERMODE_OBJS))

.PHONY: all clean qemu

all: $(TARGET).iso

qemu: $(TARGET).iso
	# Boot order "dc": boot first from cdrom (d), then from hard disk (c)
	qemu-system-i386 -cdrom $^ $(QEMU_FLAGS)

# Make a bootable ISO file
$(TARGET).iso: $(TARGET) $(KERNEL_SRC_DIR)/grub.cfg $(INITRD)
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(TARGET) $(BUILD_DIR)/isodir/boot/stos
	cp $(KERNEL_SRC_DIR)/grub.cfg $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/isodir

# Build all .c and .s source files and link them to the output file
$(TARGET): $(KERNEL_OBJS) $(LIBK_TARGET)
	$(CC) -o $@ $^ $(LDFLAGS)
	grub-file --is-x86-multiboot2 $@ && echo "Multiboot validated"

# Kernel .o rules
$(BUILD_DIR)/%.o: $(KERNEL_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(KERNEL_SRC_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -c $< -o $@

# Libk .libk.o rules (freestanding)
$(LIBK_BUILD_DIR)/%.libk.o: $(LIBC_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(LIBK_CFLAGS) -c $< -o $@

$(LIBK_BUILD_DIR)/%.libk.o: $(LIBC_SRC_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -c $< -o $@

$(LIBK_TARGET): $(LIBK_OBJS)
	@mkdir -p $(LIBK_BUILD_DIR)
	$(AR) rcs $@ $^

# Building initrd memory by creating a tar file with all required files.
# Temporary solution: just tar the grub.cfg file and few other files
$(INITRD): $(USERMODE_BIN)
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	tar -C $(USERMODE_BUILD_DIR) -cf $@ $(notdir $(USERMODE_BIN))

$(USERMODE_BUILD_DIR)/%: $(USERMODE_CRT0_OBJ) $(USERMODE_BUILD_DIR)/%.u.o
	$(CC) -o $@ $^ $(USERMODE_LDFLAGS)

$(USERMODE_BUILD_DIR)/%.u.o: $(USERMODE_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(USERMODE_CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
