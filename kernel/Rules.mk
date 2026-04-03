CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra $(LIB_INCLUDE_FLAGS) -I$(KERNEL_SRC_DIR)/include
LDFLAGS := -T $(KERNEL_SRC_DIR)/arch/i386/linker.ld -ffreestanding -O2 -nostdlib -lgcc -z max-page-size=0x1000

KERNEL_C    := $(shell find $(KERNEL_SRC_DIR)/kernel $(KERNEL_SRC_DIR)/arch -name '*.c')
KERNEL_S    := $(shell find $(KERNEL_SRC_DIR)/kernel $(KERNEL_SRC_DIR)/arch -name '*.s')
KERNEL_SRCS := $(KERNEL_C) $(KERNEL_S)

KERNEL_OBJS := $(patsubst $(KERNEL_SRC_DIR)/%,$(BUILD_DIR)/%,$(KERNEL_SRCS:.c=.o))
KERNEL_OBJS := $(patsubst $(KERNEL_SRC_DIR)/%,$(BUILD_DIR)/%,$(KERNEL_OBJS:.s=.o))

$(BUILD_DIR)/%.o: $(KERNEL_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(KERNEL_SRC_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) -c $< -o $@

$(TARGET): $(KERNEL_OBJS) $(LIBK_TARGET) $(LIBDS_TARGET)
	$(CC) -o $@ $^ $(LDFLAGS)
	grub-file --is-x86-multiboot2 $@ && echo "Multiboot validated"
