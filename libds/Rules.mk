LIBDS_SRC_DIR   := $(LIBDS_DIR)/src
LIBDS_BUILD_DIR := $(BUILD_DIR)/lib/libds
LIBDS_TARGET := $(BUILD_DIR)/lib/libds.a

LIBDS_CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror $(LIB_INCLUDE_FLAGS)

LIBDS_C    := $(shell find $(LIBDS_SRC_DIR) -name '*.c')
LIBDS_S    := $(shell find $(LIBDS_SRC_DIR) -name '*.s')
LIBDS_SRCS := $(LIBDS_C) $(LIBDS_S)

LIBDS_TEST_DIR    := $(LIBDS_DIR)/tests
LIBDS_TEST_BUILD  := $(BUILD_DIR)/tests/libds
LIBDS_TEST_TARGET := $(BUILD_DIR)/tests/libds_test

HOST_CC         := gcc
HOST_CFLAGS     := -std=gnu99 -O2 -Wall -Wextra -I$(LIBDS_DIR)/include

LIBDS_TEST_C    := $(shell find $(LIBDS_TEST_DIR) -name '*.c')
LIBDS_TEST_OBJS := \
    $(patsubst $(LIBDS_TEST_DIR)/%.c,$(LIBDS_TEST_BUILD)/tests/%.o,$(LIBDS_TEST_C)) \
    $(patsubst $(LIBDS_SRC_DIR)/%.c,$(LIBDS_TEST_BUILD)/src/%.o,$(LIBDS_C))

$(LIBDS_TEST_BUILD)/tests/%.o: $(LIBDS_TEST_DIR)/%.c
	@mkdir -p $(@D)
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(LIBDS_TEST_BUILD)/src/%.o: $(LIBDS_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(LIBDS_TEST_TARGET): $(LIBDS_TEST_OBJS)
	@mkdir -p $(@D)
	$(HOST_CC) $(HOST_CFLAGS) $^ -o $@

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
