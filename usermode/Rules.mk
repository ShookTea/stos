USERMODE_SRC_DIR   := usermode
USERMODE_BUILD_DIR := $(BUILD_DIR)/usermode

USERMODE_CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I$(LIBC_DIR)/include
USERMODE_LDFLAGS := -T $(USERMODE_SRC_DIR)/linker.ld -nostdlib -ffreestanding -lgcc

USERMODE_C    := $(shell find $(USERMODE_SRC_DIR) -name '*.c' ! -name 'crt0.c')
USERMODE_SRCS := $(USERMODE_C)

USERMODE_CRT0     := $(USERMODE_SRC_DIR)/crt0.c
USERMODE_CRT0_OBJ := $(USERMODE_BUILD_DIR)/crt0.u.o

USERMODE_OBJS := $(patsubst $(USERMODE_SRC_DIR)/%,$(USERMODE_BUILD_DIR)/%,$(USERMODE_SRCS:.c=.u.o))
USERMODE_BIN  := $(patsubst $(USERMODE_BUILD_DIR)/%.u.o,$(USERMODE_BUILD_DIR)/%,$(USERMODE_OBJS))

$(INITRD): $(USERMODE_BIN)
	mkdir -p $(@D)
	tar -C $(USERMODE_BUILD_DIR) -cf $@ $(notdir $(USERMODE_BIN))

$(USERMODE_BUILD_DIR)/%: $(USERMODE_CRT0_OBJ) $(USERMODE_BUILD_DIR)/%.u.o $(LIBC_TARGET)
	$(CC) -o $@ $^ $(USERMODE_LDFLAGS)

$(USERMODE_BUILD_DIR)/%.u.o: $(USERMODE_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(USERMODE_CFLAGS) -c $< -o $@
