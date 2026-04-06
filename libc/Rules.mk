LIBC_SRC_DIR   := $(LIBC_DIR)/src
LIBK_BUILD_DIR := $(BUILD_DIR)/lib/libk
LIBC_BUILD_DIR := $(BUILD_DIR)/lib/libc

LIBK_TARGET := $(BUILD_DIR)/lib/libk.a
LIBC_TARGET := $(BUILD_DIR)/lib/libc.a

LIBC_CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror $(LIB_INCLUDE_FLAGS)
LIBK_CFLAGS := $(LIBC_CFLAGS) -I$(KERNEL_SRC_DIR)/include -D__is_libk

LIBC_C    := $(shell find $(LIBC_SRC_DIR) -name '*.c')
LIBC_S    := $(shell find $(LIBC_SRC_DIR) -name '*.s')
LIBC_SRCS := $(LIBC_C) $(LIBC_S)

LIBK_OBJS := $(patsubst $(LIBC_SRC_DIR)/%,$(LIBK_BUILD_DIR)/%,$(LIBC_SRCS:.c=.libk.o))
LIBK_OBJS := $(patsubst $(LIBC_SRC_DIR)/%,$(LIBK_BUILD_DIR)/%,$(LIBK_OBJS:.s=.libk.o))
LIBC_OBJS := $(patsubst $(LIBC_SRC_DIR)/%,$(LIBC_BUILD_DIR)/%,$(LIBC_SRCS:.c=.libc.o))
LIBC_OBJS := $(patsubst $(LIBC_SRC_DIR)/%,$(LIBC_BUILD_DIR)/%,$(LIBC_OBJS:.s=.libc.o))

$(LIBK_BUILD_DIR)/%.libk.o: $(LIBC_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(LIBK_CFLAGS) -c $< -o $@

$(LIBK_BUILD_DIR)/%.libk.o: $(LIBC_SRC_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) -c $< -o $@

$(LIBK_TARGET): $(LIBK_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^

$(LIBC_BUILD_DIR)/%.libc.o: $(LIBC_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(LIBC_CFLAGS) -c $< -o $@

$(LIBC_BUILD_DIR)/%.libc.o: $(LIBC_SRC_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) -c $< -o $@

$(LIBC_TARGET): $(LIBC_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^
