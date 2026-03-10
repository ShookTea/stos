# Compiler / tools
CC      := i686-elf-gcc
AS      := i686-elf-as
AR      := i686-elf-ar
CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Isrc/libc/include -Isrc/include
ASFLAGS := # asm flags if needed
LDFLAGS := -T src/arch/i386/linker.ld -ffreestanding -O2 -nostdlib -lgcc -z max-page-size=0x1000
LIBK_CFLAGS := $(CFLAGS) -D__is_libk
TARGET := build/stos
LIB := build/lib/libk.a

# Directories
SRC_DIR := src
BUILD_DIR := build
LIB_DIR := $(BUILD_DIR)/lib

# Kernel sources (exluding libc/libk)
KERNEL_C := $(shell find $(SRC_DIR)/kernel $(SRC_DIR)/arch -name '*.c')
KERNEL_S := $(shell find $(SRC_DIR)/kernel $(SRC_DIR)/arch -name '*.s')
KERNEL_SRCS := $(KERNEL_C) $(KERNEL_S)

# Libk sources
LIBK_C := $(shell find $(SRC_DIR)/libc -name '*.c')
LIBK_S := $(shell find $(SRC_DIR)/libc -name '*.s')
LIBK_SRCS := $(LIBK_C) $(LIBK_S)

# Convert e.g. kernel/foo/bar.c -> build/foo/bar.o
KERNEL_OBJS := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(KERNEL_SRCS:.c=.o))
KERNEL_OBJS := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(KERNEL_OBJS:.s=.o))
LIBK_OBJS := $(patsubst $(SRC_DIR)/%,$(LIB_DIR)/%,$(LIBK_SRCS:.c=.libk.o))
LIBK_OBJS := $(patsubst $(SRC_DIR)/%,$(LIB_DIR)/%,$(LIBK_OBJS:.s=.libk.o))

.PHONY: all clean libk

all: $(TARGET).iso

qemu: $(TARGET).iso
	qemu-system-i386 -cdrom $^ -m 512M -serial stdio

# Make a bootable ISO file
$(TARGET).iso: $(TARGET) src/grub.cfg
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(TARGET) $(BUILD_DIR)/isodir/boot/stos
	cp src/grub.cfg $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/isodir

# Build all .c and .s source files and link them to the output file
$(TARGET): $(KERNEL_OBJS) $(LIB)
	$(CC) -o $@ $^ $(LDFLAGS)
	grub-file --is-x86-multiboot2 $@ && echo "Multiboot validated"

# Kernel .o rules
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -c $< -o $@

# Libk .libk.o rules (freestanding)
$(LIB_DIR)/%.libk.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(LIBK_CFLAGS) -c $< -o $@

$(LIB_DIR)/%.libk.o: $(SRC_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

$(LIB): $(LIBK_OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

clean:
	rm -rf $(BUILD_DIR)
