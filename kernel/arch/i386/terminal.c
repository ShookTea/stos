#include <kernel/terminal.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel/serial.h>
#include <kernel/memory/kmalloc.h>
#include <stdio.h>
#include "kernel/drivers/vga/fbcon.h"
#include "kernel/drivers/vga/rgb.h"
#include "kernel/multiboot2.h"
#include "kernel/drivers/vga/text_mode.h"
#include <ctype.h>
#include <stdlib.h>
#include "kernel/debug.h"

#define TERMINAL_TAB_ALIGN 8
// Space character created for tab alignment
#define CHARFLAG_TABSPACE 0x01

#define BG_COLOR_DEFAULT VGA_COLOR_BLACK
#define FG_COLOR_DEFAULT VGA_COLOR_LIGHT_GREY

#define INTENSITY_MODE_NORMAL 0
#define INTENSITY_MODE_BOLD 1

/**
 * Structure describing a single character on screen
 */
typedef struct {
    uint32_t codepoint; // displayed character
    enum vga_color bg_color;
    enum vga_color fg_color;
    uint8_t flags;
} cell_t;

static size_t vga_width;
static size_t vga_height;

// Current cursor location
static size_t cursor_row;
static size_t cursor_column;

// Currently selected color values
static enum vga_color bg_color;
static enum vga_color fg_color;

static cell_t* cell_buffer;

static bool initialized = false;

// Received \033 escape code
static bool in_escape_mode = false;
// Control Sequence Introducer
static bool escape_mode_csi = false;
static char* escape_mode_buffer = NULL;
static size_t escape_mode_buffer_length = 0;

static size_t saved_cursor_row = 0;
static size_t saved_cursor_column = 0;
static uint8_t font_style;

static bool rgbmode = false;

/**
 * Merges bg_color and fg_color into entry acceptable by VGA.
 */
static inline uint8_t get_current_color()
{
    return fg_color | bg_color << 4;
}

static void putentryat(uint32_t c, size_t row, size_t column)
{
    if (rgbmode) {
        fbcon_putentryat(c, (uint8_t)fg_color, (uint8_t)bg_color, column, row);
    } else {
        vga_text_putentryat(
            c,
            get_current_color(),
            row,
            column
        );
    }
}

static void set_cursor_position(size_t row, size_t column)
{
    rgbmode
        ? fbcon_set_cursor_position(column, row)
        : vga_text_set_cursor_position(row, column);
}

static void erase_at_pos(size_t row, size_t column)
{
    size_t index = row * vga_width + column;
    cell_buffer[index].codepoint = '\0';
    cell_buffer[index].flags = 0;
    cell_buffer[index].bg_color = bg_color;
    cell_buffer[index].fg_color = fg_color;
    putentryat(' ', row, column);
}

/**
 * Scroll one line up, adding new line at the bottom
 */
static void terminal_scroll_up()
{
    rgbmode ? fbcon_scroll_up() : vga_text_scroll_up();
    // Move characters up by line
    for (size_t i = 0; i < (vga_height - 1) * vga_width; i++) {
        cell_buffer[i] = cell_buffer[i + vga_width];
    }
    // Clear last line
    for (size_t c = 0; c < vga_width; c++) {
        erase_at_pos(vga_height - 1, c);
    }
}

static void terminal_scroll_down()
{
    rgbmode ? fbcon_scroll_down() : vga_text_scroll_down();
    // Move characters down by line
    for (size_t i = vga_height * vga_width - 1; i >= vga_width; i--) {
        cell_buffer[i] = cell_buffer[i - vga_width];
    }
    // Clear first line
    for (size_t c = 0; c < vga_width; c++) {
        erase_at_pos(0, c);
    }
}

static void terminal_reset_styling()
{
    bg_color = BG_COLOR_DEFAULT;
    fg_color = FG_COLOR_DEFAULT;
    font_style = INTENSITY_MODE_NORMAL;
}

/**
 * Every CSI sequence is a set of zero, one, or more numberical arguments
 * (separated by a semicolon), followed by the mode letter.
 */
static void terminal_handle_csi_sequence()
{
    uint8_t* args = NULL; // array of arguments
    size_t arg_count = 0;
    char mode = '\0'; // Mode letter
    char* buffer = NULL;
    size_t buffer_length = 0;

    for (size_t i = 0; i < escape_mode_buffer_length; i++) {
        char next_char = escape_mode_buffer[i];
        if (i == 0 && next_char == '?') {
            // Ignore question marks
            continue;
        }
        if (isdigit(next_char)) {
            // This is a digit - add it to the buffer
            buffer = krealloc(
                buffer,
                sizeof(char) * (buffer_length + 2)
            );
            buffer[buffer_length] = next_char;
            buffer[buffer_length + 1] = '\0';
            buffer_length++;
        }
        else {
            // Either a semicolon (arg separator) or a mode character
            // 1. Parse current buffer, if exists - otherwise add 0
            args = krealloc(args, sizeof(uint8_t) * (arg_count + 1));
            args[arg_count] = buffer_length > 0 ? atoi(buffer) : 0;
            arg_count++;
            if (buffer_length > 0) {
                kfree(buffer);
                buffer = NULL;
                buffer_length = 0;
            }

            // 2. Load mode
            if (next_char != ';') {
                mode = next_char;
                break;
            }
        }
    }

    if (mode == 'A') {
        // Move cursor N cells up
        if (args[0] == 0) {
            args[0] = 1;
        }
        if (args[0] >= cursor_row) {
            cursor_row = 0;
        } else {
            cursor_row -= args[0];
        }
        set_cursor_position(cursor_row, cursor_column);
    }
    else if (mode == 'B') {
        // Move cursor N cells down
        if (args[0] == 0) {
            args[0] = 1;
        }
        if (args[0] + cursor_row >= vga_height) {
            cursor_row = vga_height - 1;
        } else {
            cursor_row += args[0];
        }
        set_cursor_position(cursor_row, cursor_column);
    }
    else if (mode == 'C') {
        // Move cursor N cells forward
        if (args[0] == 0) {
            args[0] = 1;
        }
        if (args[0] + cursor_column >= vga_width) {
            cursor_column = vga_width - 1;
        } else {
            cursor_column += args[0];
        }
        set_cursor_position(cursor_row, cursor_column);
    }
    else if (mode == 'D') {
        // Move cursor N cells back
        if (args[0] == 0) {
            args[0] = 1;
        }
        if (args[0] >= cursor_column) {
            cursor_column = 0;
        } else {
            cursor_column -= args[0];
        }
        set_cursor_position(cursor_row, cursor_column);
    }
    else if (mode == 'E') {
        // Move cursor to beginning of line, N rows down
        if (args[0] == 0) {
            args[0] = 1;
        }
        if (args[0] + cursor_row >= vga_height) {
            cursor_row = vga_height - 1;
        } else {
            cursor_row += args[0];
        }
        cursor_column = 0;
        set_cursor_position(cursor_row, cursor_column);
    }
    else if (mode == 'F') {
        // Move cursor to beginning of line, N rows up
        if (args[0] == 0) {
            args[0] = 1;
        }
        if (args[0] >= cursor_row) {
            cursor_row = 0;
        } else {
            cursor_row -= args[0];
        }
        cursor_column = 0;
        set_cursor_position(cursor_row, cursor_column);
    }
    else if (mode == 'G') {
        // Move cursor to column N (starting from 1)
        if (args[0] == 0) {
            args[0] = 1;
        }
        args[0]--;
        if (args[0] >= vga_width) {
            cursor_column = vga_width - 1;
        } else {
            cursor_column = args[0];
        }
        set_cursor_position(cursor_row, cursor_column);
    }
    else if (mode == 'H') {
        // Move cursor to row arg[0], column arg[1] (starting from 1)
        uint8_t row, column;
        if (args[0] == 0) {
            row = 0;
        } else {
            row = args[0] - 1;
        }
        if (arg_count < 2 || args[1] == 0) {
            column = 0;
        } else {
            column = args[1] - 1;
        }

        if (row >= vga_height) {
            cursor_row = vga_height - 1;
        } else {
            cursor_row = row;
        }

        if (column >= vga_width) {
            cursor_column = vga_width - 1;
        } else {
            cursor_column = column;
        }
        set_cursor_position(cursor_row, cursor_column);
    }
    else if (mode == 'J') {
        // Erase in display
        if (args[0] == 0) {
            // Clear from cursor to end of screen
            // First erase to the end of current line
            for (size_t c = cursor_column; c < vga_width; c++) {
                erase_at_pos(cursor_row, c);
            }
            // Then erase the rest of rows:
            for (size_t r = cursor_row + 1; r < vga_height; r++) {
                for (size_t c = 0; c < vga_width; c++) {
                    erase_at_pos(r, c);
                }
            }
        }
        else if (args[0] == 1) {
            // Clear from cursor to the start of screen
            // First erase to the start of current line
            for (size_t c = 0; c <= cursor_column; c++) {
                erase_at_pos(cursor_row, c);
            }
            // Then erase the rest of rows:
            for (size_t r = 0; r < cursor_row; r++) {
                for (size_t c = 0; c < vga_width; c++) {
                    erase_at_pos(r, c);
                }
            }
        }
        else if (args[0] == 2) {
            // Clear the entire display
            for (size_t r = 0; r < vga_height; r++) {
                for (size_t c = 0; c < vga_width; c++) {
                    erase_at_pos(r, c);
                }
            }
        }
    }
    else if (mode == 'K') {
        // Erase in line
        if (args[0] == 0) {
            // Clear from cursor to end of line
            for (size_t c = cursor_column; c < vga_width; c++) {
                erase_at_pos(cursor_row, c);
            }
        }
        else if (args[0] == 1) {
            // Clear from cursor to the start of line
            for (size_t c = 0; c <= cursor_column; c++) {
                erase_at_pos(cursor_row, c);
            }
        }
        else if (args[0] == 2) {
            // Clear the entire line
            for (size_t c = 0; c < vga_width; c++) {
                erase_at_pos(cursor_row, c);
            }
        }
    }
    else if (mode == 'S') {
        // Scroll up
        if (args[0] == 0) {
            args[0] = 1;
        }
        for (size_t i = 0; i < args[0]; i++) {
            terminal_scroll_up();
        }
    }
    else if (mode == 'T') {
        // Scroll down
        if (args[0] == 0) {
            args[0] = 1;
        }
        for (size_t i = 0; i < args[0]; i++) {
            terminal_scroll_down();
        }
    }
    else if (mode == 's') {
        // Save current cursor position
        saved_cursor_row = cursor_row;
        saved_cursor_column = cursor_column;
    }
    else if (mode == 'u') {
        // Restore saved cursor position
        cursor_row = saved_cursor_row;
        cursor_column = saved_cursor_column;
        set_cursor_position(cursor_row, cursor_column);
    }
    else if (mode == 'h') {
        if (args[0] == 25) {
            rgbmode ? fbcon_enable_cursor() : vga_text_enable_cursor();
        }
    }
    else if (mode == 'l') {
        if (args[0] == 25) {
            rgbmode ? fbcon_disable_cursor() : vga_text_disable_cursor();
        }
    }
    else if (mode == 'm') {
        // Graphic rendition mode - parse each arg one by one
        for (size_t i = 0; i < arg_count; i++) {
            if (args[i] == 0) {
                // Reset rendition rules
                terminal_reset_styling();
            }
            else if (args[i] == 1) {
                font_style = INTENSITY_MODE_BOLD;
            }
            else if (args[i] == 22) {
                font_style = INTENSITY_MODE_NORMAL;
            }
            else if ((args[i] >= 30 && args[i] <= 37)
                || (args[i] >= 40 && args[i] <= 47)) {
                // Set bg/fg color
                bool foreground = args[i] < 40;
                enum vga_color color;
                switch (args[i] % 10) {
                    case 0: color = VGA_COLOR_BLACK; break;
                    case 1: color = VGA_COLOR_RED; break;
                    case 2: color = VGA_COLOR_GREEN; break;
                    case 3: color = VGA_COLOR_BROWN; break;
                    case 4: color = VGA_COLOR_BLUE; break;
                    case 5: color = VGA_COLOR_MAGENTA; break;
                    case 6: color = VGA_COLOR_CYAN; break;
                    case 7: color = VGA_COLOR_LIGHT_GREY; break;
                }
                if (foreground) {
                    fg_color = color;
                } else {
                    bg_color = color;
                }
            }
            else if (args[i] == 39) {
                // Set default foreground color
                fg_color = FG_COLOR_DEFAULT;
            }
            else if (args[i] == 49) {
                // Set default background color
                bg_color = BG_COLOR_DEFAULT;
            }
            else if ((args[i] >= 90 && args[i] <= 97)
                || (args[i] >= 100 && args[i] <= 107)) {
                // Set bg/fg color (intense variant)
                bool foreground = args[i] < 100;
                enum vga_color color;
                switch (args[i] % 10) {
                    case 0: color = VGA_COLOR_DARK_GREY; break;
                    case 1: color = VGA_COLOR_LIGHT_RED; break;
                    case 2: color = VGA_COLOR_LIGHT_GREEN; break;
                    case 3: color = VGA_COLOR_LIGHT_BROWN; break;
                    case 4: color = VGA_COLOR_LIGHT_BLUE; break;
                    case 5: color = VGA_COLOR_LIGHT_MAGENTA; break;
                    case 6: color = VGA_COLOR_LIGHT_CYAN; break;
                    case 7: color = VGA_COLOR_WHITE; break;
                }
                if (foreground) {
                    fg_color = color;
                } else {
                    bg_color = color;
                }
            }
        }
    }

    if (buffer != NULL) {
        kfree(buffer);
    }
    if (args != NULL) {
        kfree(args);
    }
}

static void terminal_reset_escape_mode_state()
{
    in_escape_mode = false;
    escape_mode_csi = false;
    if (escape_mode_buffer_length > 0) {
        kfree(escape_mode_buffer);
        escape_mode_buffer = NULL;
        escape_mode_buffer_length = 0;
    }
}

static void terminal_detect_mode()
{
    framebuffer_rgb_config_t cfg;
    multiboot2_load_framebuffer_rgb_config(&cfg);
    rgbmode = cfg.enabled;
}

void terminal_init()
{
    initialized = true;
    terminal_detect_mode();
    terminal_reset_styling();
    if (rgbmode) {
        vga_rgb_init();
        fbcon_init();
        vga_width = fbcon_get_columns();
        vga_height = fbcon_get_rows();
    } else {
        vga_text_init(get_current_color());
        vga_text_disable_cursor();
        vga_width = vga_text_get_columns();
        vga_height = vga_text_get_rows();
    }

    cell_buffer = kmalloc_flags(
        vga_width * vga_height * sizeof(cell_t),
        KMALLOC_ZERO
    );
}

void terminal_write_char(char c)
{
    #if KERNEL_DEBUG_COM
        serial_put_c(c);
    #endif
    if (!initialized) {
        return;
    }

    static uint32_t utf8 = 0;
    static int utf8_remaining = 0;

    if (c & 0x80) {
        uint8_t v = c;
        // This is a part of unicode character.
        if (v >= 0xC0 && v <= 0xDF) {
            // 2-byte lead
            utf8 = v & 0x1F;
            utf8_remaining = 1;
        } else if (v >= 0xE0 && v <= 0xEF) {
            // 3-byte lead
            utf8 = v & 0x0F;
            utf8_remaining = 2;
        } else if (v >= 0xF0 && v <= 0xF7) {
            // 4-byte lead
            utf8 = v & 0x07;
            utf8_remaining = 3;
        } else {
            // Continuation - accumulate
            utf8 = (utf8 << 6) | (v & 0x3F);
            utf8_remaining--;
        }
    } else {
        utf8 = c;
    }
    if (utf8_remaining > 0) {
        return;
    }

    if (!in_escape_mode && c == '\033') {
        // Entering escape mode
        in_escape_mode = true;
        return;
    }

    if (in_escape_mode) {
        if (!escape_mode_csi && c == '[') {
            escape_mode_csi = true;
        }
        else if (escape_mode_csi && c >= 0x20 && c <= 0x3F) {
            // Values between 0x20 and 0x3F are valid "middle" values in CSI.
            // Append them to buffer.
            escape_mode_buffer = krealloc(
                escape_mode_buffer,
                sizeof(char) * (escape_mode_buffer_length + 2)
            );
            escape_mode_buffer[escape_mode_buffer_length] = c;
            escape_mode_buffer[escape_mode_buffer_length + 1] = '\0';
            escape_mode_buffer_length++;
        }
        else if (escape_mode_csi && c >= 0x40 && c <= 0x7E) {
            // A value between 0x40 and 0x7E is a valid end byte in CSI.
            // Append them to buffer, then handle CSI escape sequence.
            escape_mode_buffer = krealloc(
                escape_mode_buffer,
                sizeof(char) * (escape_mode_buffer_length + 2)
            );
            escape_mode_buffer[escape_mode_buffer_length] = c;
            escape_mode_buffer[escape_mode_buffer_length + 1] = '\0';
            escape_mode_buffer_length++;
            terminal_handle_csi_sequence();
            terminal_reset_escape_mode_state();
        }
        else {
            // Unrecognized situation - reset escape mode state
            terminal_reset_escape_mode_state();
        }
        return;
    }

    if (c == '\b') {
        // Remove previous character(s)
        if (cursor_column == 0 && cursor_row > 0) {
            // Move to end of previous line
            cursor_column = vga_width - 1;
            cursor_row--;
        }
        else if (cursor_column > 0) {
            cursor_column--;
        }
        else {
            // At top-left corner, nothing to delete
            return;
        }

        size_t index = cursor_row * vga_width + cursor_column;

        // Check if we're deleting a tab-created space
        if (cell_buffer[index].flags & CHARFLAG_TABSPACE) {
            // Remove all contiguous tab spaces backwards
            while (cursor_column > 0 &&
                   (cell_buffer[index].flags & CHARFLAG_TABSPACE)) {
                cell_buffer[index].codepoint = '\0';
                cell_buffer[index].flags = 0;
                #if KERNEL_DEBUG_COM
                    serial_put_c('\b');
                #endif
                putentryat(
                    ' ',
                    cursor_row,
                    cursor_column
                );
                cursor_column--;
                index--;
            }
        }

        // Clear the character at current position
        erase_at_pos(cursor_row, cursor_column);
        #if KERNEL_DEBUG_COM
            serial_put_c(' ');
            serial_put_c('\b');
        #endif

        set_cursor_position(cursor_row, cursor_column);
        return;
    }

    // =================================
    // Handling drawable characters
    // =================================

    // TODO: handle special characters and escape codes
    size_t index = cursor_row * vga_width + cursor_column;
    cell_buffer[index].codepoint = c;
    cell_buffer[index].bg_color = bg_color;
    cell_buffer[index].fg_color = fg_color;
    cell_buffer[index].flags = 0;


    if (c == '\n') {
        // Handle new line: move cursor one line below
        cursor_column = 0;
        if (++cursor_row >= vga_height) {
            cursor_row = vga_height - 1;
            terminal_scroll_up();
        }
    }
    else if (c == '\t') {
        size_t new_column = cursor_column
            + (TERMINAL_TAB_ALIGN - cursor_column % TERMINAL_TAB_ALIGN);
        if (new_column >= vga_width) {
            new_column = vga_width;
        }
        size_t index_offset = 0;
        for (size_t i = cursor_column; i < new_column; i++) {
            putentryat(
                ' ',
                cursor_row,
                cursor_column
            );
            if (index_offset > 0) {
                cell_buffer[index + index_offset].codepoint = ' ';
                cell_buffer[index + index_offset].bg_color = bg_color;
                cell_buffer[index + index_offset].fg_color = fg_color;
                cell_buffer[index + index_offset].flags = CHARFLAG_TABSPACE;
            }
            index_offset++;
        }

        cursor_column = new_column;

        if (cursor_column == vga_width) {
            cursor_column = 0;
            if (++cursor_row >= vga_height) {
                cursor_row = vga_height - 1;
                terminal_scroll_up();
            }
        }
    }
    else {
        // Display normal character
        putentryat(
            utf8,
            cursor_row,
            cursor_column
        );

        if (++cursor_column == vga_width) {
            cursor_column = 0;
            if (++cursor_row >= vga_height) {
                cursor_row = vga_height - 1;
                terminal_scroll_up();
            }
        }
    }

    set_cursor_position(cursor_row, cursor_column);
}

void terminal_set_bg_color(enum vga_color color)
{
    bg_color = color;
}

void terminal_set_fg_color(enum vga_color color)
{
    fg_color = color;
}
