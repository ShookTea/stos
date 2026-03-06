# Compiler / tools
CC      := i686-elf-gcc
AS      := i686-elf-as
CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra
ASFLAGS := # asm flags if needed
LDFLAGS := -T kernel/arch/i386/linker.ld -ffreestanding -O2 -nostdlib -lgcc
TARGET  := build/stos

# Directories
SRC_DIR := kernel
BUILD_DIR := build

# Find all C and asm sources recursively
SRCS_C  := $(shell find $(SRC_DIR) -name '*.c')
SRCS_S  := $(shell find $(SRC_DIR) -name '*.s')
SRCS    := $(SRCS_C) $(SRCS_S)

# Convert e.g. kernel/foo/bar.c -> build/foo/bar.o
OBJS := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SRCS:.c=.o))
OBJS := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(OBJS:.s=.o))

.PHONY: all clean

all: $(TARGET)

# Build all .c and .s source files and link them to the output file
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	grub-file --is-x86-multiboot $@ && echo "Multiboot validated"

# Pattern rule: build/whatever.o from src/whatever.c or src/whatever.s
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
