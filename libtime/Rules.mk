LIBTIME_SRC_DIR   := $(LIBTIME_DIR)/src
LIBTIME_BUILD_DIR := $(BUILD_DIR)/lib/LIBTIME
LIBTIME_TARGET := $(BUILD_DIR)/lib/LIBTIME.a

LIBTIME_CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror $(LIB_INCLUDE_FLAGS)

LIBTIME_C    := $(shell find $(LIBTIME_SRC_DIR) -name '*.c')
LIBTIME_S    := $(shell find $(LIBTIME_SRC_DIR) -name '*.s')
LIBTIME_SRCS := $(LIBTIME_C) $(LIBTIME_S)

LIBTIME_TEST_DIR    := $(LIBTIME_DIR)/tests
LIBTIME_TEST_BUILD  := $(BUILD_DIR)/tests/libtime
LIBTIME_TEST_TARGET := $(BUILD_DIR)/tests/libtime_test

LIBTIME_HOST_CC     := gcc
LIBTIME_HOST_CFLAGS := -std=gnu99 -O2 -Wall -Wextra -I$(LIBTIME_DIR)/include

LIBTIME_TEST_C    := $(shell find $(LIBTIME_TEST_DIR) -name '*.c')
LIBTIME_TEST_OBJS := \
    $(patsubst $(LIBTIME_TEST_DIR)/%.c,$(LIBTIME_TEST_BUILD)/tests/%.o,$(LIBTIME_TEST_C)) \
    $(patsubst $(LIBTIME_SRC_DIR)/%.c,$(LIBTIME_TEST_BUILD)/src/%.o,$(LIBTIME_C))

$(LIBTIME_TEST_BUILD)/tests/%.o: $(LIBTIME_TEST_DIR)/%.c
	@mkdir -p $(@D)
	$(LIBTIME_HOST_CC) $(LIBTIME_HOST_CFLAGS) -c $< -o $@

$(LIBTIME_TEST_BUILD)/src/%.o: $(LIBTIME_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(LIBTIME_HOST_CC) $(LIBTIME_HOST_CFLAGS) -c $< -o $@

$(LIBTIME_TEST_TARGET): $(LIBTIME_TEST_OBJS)
	@mkdir -p $(@D)
	$(LIBTIME_HOST_CC) $(LIBTIME_HOST_CFLAGS) $^ -o $@

LIBTIME_OBJS := $(patsubst $(LIBTIME_SRC_DIR)/%,$(LIBTIME_BUILD_DIR)/%,$(LIBTIME_SRCS:.c=.LIBTIME.o))
LIBTIME_OBJS := $(patsubst $(LIBTIME_SRC_DIR)/%,$(LIBTIME_BUILD_DIR)/%,$(LIBTIME_OBJS:.s=.LIBTIME.o))

$(LIBTIME_BUILD_DIR)/%.LIBTIME.o: $(LIBTIME_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(LIBTIME_CFLAGS) -c $< -o $@

$(LIBTIME_BUILD_DIR)/%.LIBTIME.o: $(LIBTIME_SRC_DIR)/%.s
	@mkdir -p $(@D)
	$(AS) -c $< -o $@

$(LIBTIME_TARGET): $(LIBTIME_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^
