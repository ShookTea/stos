#include <kernel/terminal.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel/serial.h>
#include <kernel/memory/kmalloc.h>
#include <stdio.h>
#include "vga.h"
#include <ctype.h>
#include <stdlib.h>

#define TERMINAL_TAB_ALIGN 8
// Space character created for tab alignment
#define CHARFLAG_TABSPACE 0x01

/**
 * Structure describing a single character on screen
 */
typedef struct {
    char codepoint; // displayed character
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

/**
 * Scroll one line up, adding new line at the bottom
 */
static void terminal_scroll_up()
{
    vga_scroll_up();
    // Move characters up by line
    for (size_t i = 0; i < (vga_height - 1) * vga_width; i++) {
        cell_buffer[i] = cell_buffer[i + vga_width];
    }
    // Clear last line
    for (
        size_t i = (vga_height - 1) * vga_width;
        i < vga_height * vga_width;
        i++
    ) {
        cell_buffer[i].codepoint = '\0';
        cell_buffer[i].bg_color = bg_color;
        cell_buffer[i].fg_color = fg_color;
        cell_buffer[i].flags = 0;
    }
}

static void terminal_scroll_down()
{
    vga_scroll_down();
    // Move characters down by line
    for (size_t i = (vga_height - 1) * vga_width - 1; i >= vga_width; i--) {
        cell_buffer[i] = cell_buffer[i - vga_width];
    }
    // Clear first line
    for (size_t i = 0; i < vga_width; i++) {
        cell_buffer[i].codepoint = '\0';
        cell_buffer[i].bg_color = bg_color;
        cell_buffer[i].fg_color = fg_color;
        cell_buffer[i].flags = 0;
    }
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
        vga_set_cursor_position(cursor_row, cursor_column);
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
        vga_set_cursor_position(cursor_row, cursor_column);
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
        vga_set_cursor_position(cursor_row, cursor_column);
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
        vga_set_cursor_position(cursor_row, cursor_column);
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
        vga_set_cursor_position(cursor_row, cursor_column);
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
        vga_set_cursor_position(cursor_row, cursor_column);
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
        vga_set_cursor_position(cursor_row, cursor_column);
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
        vga_set_cursor_position(cursor_row, cursor_column);
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

void terminal_init()
{
    initialized = true;
    bg_color = VGA_COLOR_BLACK;
    fg_color = VGA_COLOR_LIGHT_GREY;
    vga_init(vga_entry_color(fg_color, bg_color));
    vga_disable_cursor();
    vga_width = vga_get_columns();
    vga_height = vga_get_rows();

    cell_buffer = kmalloc_flags(
        vga_width * vga_height * sizeof(cell_t),
        KMALLOC_ZERO
    );
}

void terminal_write_char(char c)
{
    serial_put_c(c);
    if (!initialized) {
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
                vga_putentryat(
                    ' ',
                    vga_entry_color(fg_color, bg_color),
                    cursor_row,
                    cursor_column
                );
                cursor_column--;
                index--;
            }
        }

        // Clear the character at current position
        cell_buffer[index].codepoint = '\0';
        cell_buffer[index].flags = 0;
        vga_putentryat(
            ' ',
            vga_entry_color(fg_color, bg_color),
            cursor_row,
            cursor_column
        );

        vga_set_cursor_position(cursor_row, cursor_column);
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
            vga_putentryat(
                ' ',
                vga_entry_color(fg_color, bg_color),
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
        vga_putentryat(
            c,
            vga_entry_color(fg_color, bg_color),
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

    vga_set_cursor_position(cursor_row, cursor_column);
}

void terminal_enable_cursor()
{
    vga_enable_cursor();
}

void terminal_disable_cursor()
{
    vga_disable_cursor();
}

void terminal_set_bg_color(enum vga_color color)
{
    bg_color = color;
}

void terminal_set_fg_color(enum vga_color color)
{
    fg_color = color;
}
