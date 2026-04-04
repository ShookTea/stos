USERMODE_SRC_DIR   := usermode
DEP_SRC_DIR := dep
USERMODE_BUILD_DIR := $(BUILD_DIR)/usermode

USERMODE_CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra $(LIB_INCLUDE_FLAGS)
USERMODE_LDFLAGS := -T $(USERMODE_SRC_DIR)/linker.ld -nostdlib -ffreestanding -lgcc

USERMODE_C    := $(shell find $(USERMODE_SRC_DIR) -name '*.c' ! -name 'crt0.c')
USERMODE_SRCS := $(USERMODE_C)

USERMODE_CRT0     := $(USERMODE_SRC_DIR)/crt0.c
USERMODE_CRT0_OBJ := $(USERMODE_BUILD_DIR)/crt0.u.o

USERMODE_OBJS := $(patsubst $(USERMODE_SRC_DIR)/%,$(USERMODE_BUILD_DIR)/%,$(USERMODE_SRCS:.c=.u.o))
USERMODE_BIN  := $(patsubst $(USERMODE_BUILD_DIR)/%.u.o,$(USERMODE_BUILD_DIR)/%,$(USERMODE_OBJS))

PSF_FONT_SRC := $(DEP_SRC_DIR)/spleen/spleen-8x16.psfu
PSF_FONT_DEST := $(USERMODE_BUILD_DIR)/font.psf

USERMODE_TARGET = $(USERMODE_BIN) $(PSF_FONT_DEST)

$(INITRD): $(USERMODE_TARGET)
	mkdir -p $(@D)
	tar -C $(USERMODE_BUILD_DIR) -cf $@ $(notdir $(USERMODE_TARGET))

$(USERMODE_BUILD_DIR)/%: $(USERMODE_CRT0_OBJ) $(USERMODE_BUILD_DIR)/%.u.o $(LIBC_TARGET) $(LIBDS_TARGET)
	$(CC) -o $@ $^ $(USERMODE_LDFLAGS)

$(USERMODE_BUILD_DIR)/%.u.o: $(USERMODE_SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(USERMODE_CFLAGS) -c $< -o $@

$(PSF_FONT_DEST): $(PSF_FONT_SRC)
	@mkdir -p $(@D)
	cp $< $@
