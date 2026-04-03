LIBDS_SRC_DIR   := $(LIBDS_DIR)/src
LIBDS_BUILD_DIR := $(BUILD_DIR)/lib/libds
LIBDS_TARGET := $(BUILD_DIR)/lib/libds.a

LIBDS_CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra $(LIB_INCLUDE_FLAGS)

LIBDS_C    := $(shell find $(LIBDS_SRC_DIR) -name '*.c')
LIBDS_S    := $(shell find $(LIBDS_SRC_DIR) -name '*.s')
LIBDS_SRCS := $(LIBDS_C) $(LIBDS_S)

LIBDS_OBJS := $(patsubst $(LIBDS_SRC_DIR)/%,$(LIBDS_BUILD_DIR)/%,$(LIBDS_SRCS:.c=.libds.o))
LIBDS_OBJS := $(patsubst $(LIBDS_SRC_DIR)/%,$(LIBDS_BUILD_DIR)/%,$(LIBDS_OBJS:.s=.libds.o))

$(LIBDS_BUILD_DIR)/%.libds.o: $(LIBDS_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(LIBDS_CFLAGS) -c $< -o $@

$(LIBDS_BUILD_DIR)/%.libds.o: $(LIBDS_SRC_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) -c $< -o $@

$(LIBDS_TARGET): $(LIBDS_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^
