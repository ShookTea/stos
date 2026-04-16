#include "kernel/debug.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/task/wait.h"
#include "../device.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "libds/libds.h"
#include "libds/ringbuf.h"
#include <stdio.h>
#include <string.h>
#include "./tty.h"

static vfs_node_t* node = NULL;

/**
 * Buffer containing all "committed" bytes (after pressing Enter)
 */
static ds_ringbuf_t* buffer;
static size_t ready_lines = 0;
/**
 * Currently read line
 */
static char current_line[TTY_BUFFER_SIZE];
static size_t current_line_pos = 0;

static void push_curr_line_to_buffer()
{
    for (size_t i = 0; i < current_line_pos; i++) {
        ds_result_t res = ds_ringbuf_push(buffer, &current_line[i]);
        if (res == DS_ERR_FULL) {
            // Clear last character and reattempt pushing. We're doing it
            // manually instead of passing fail_on_full=false to the ringbuf
            // creation, because we need to decrent ready_lines when hitting
            // new line.
            char popped;
            ds_ringbuf_pop(buffer, &popped);
            if (popped == '\n') {
                ready_lines--;
            }
            // Re-attempt pushing
            ds_ringbuf_push(buffer, &current_line[i]);
        }
    }
    current_line_pos = 0;
    ready_lines++;
}

static inline void put_char_to_line_with_val(char line, char to_print)
{
    if (current_line_pos < TTY_BUFFER_SIZE) {
        putchar(to_print);
        current_line[current_line_pos] = line;
        current_line_pos++;
    }
}

static inline void put_char_to_line(char c)
{
    put_char_to_line_with_val(c, c);
}

static void handle_non_ascii(keyboard_event_t evt)
{
    if (evt.key_code == KCODE_LEFT_ALT || evt.key_code == KCODE_RIGHT_ALT
        || evt.key_code == KCODE_LEFT_CTRL || evt.key_code == KCODE_RIGHT_CTRL
        || evt.key_code == KCODE_LEFT_SHIFT || evt.key_code == KCODE_RIGHT_SHIFT
    ) {
        // The modifier keys themselves are never emitted.
        return;
    }

    uint8_t modifier = (0
        | ((evt.l_shift_pressed || evt.r_shift_pressed) ? 0x01 : 0)
        | (evt.l_alt_pressed ? 0x02 : 0)
        | ((evt.l_ctrl_pressed || evt.r_ctrl_pressed) ? 0x04 : 0)
        | ((evt.l_system_pressed || evt.r_system_pressed) ? 0x08 : 0)
    ) + 1;

    debug_printf_c("TTY", "keycode = %u, mod = %u\n", evt.key_code, modifier);

    if (evt.key_code == KCODE_ARROW_LEFT
        || evt.key_code == KCODE_ARROW_RIGHT
        || evt.key_code == KCODE_ARROW_UP
        || evt.key_code == KCODE_ARROW_DOWN
    ) {
        // Special case for arrow events with no modifiers added
        put_char_to_line_with_val('\033', '^');
        put_char_to_line('[');

        if (modifier != 1) {
            if (modifier >= 10) {
                put_char_to_line('0' + (modifier / 10));
                modifier %= 10;
            }
            put_char_to_line('0' + modifier);
        }

        switch (evt.key_code) {
            case KCODE_ARROW_UP: put_char_to_line('A'); break;
            case KCODE_ARROW_DOWN: put_char_to_line('B'); break;
            case KCODE_ARROW_RIGHT: put_char_to_line('C'); break;
            case KCODE_ARROW_LEFT: put_char_to_line('D'); break;
        }
    }
    else {
        // Send key in format <esc>[<keycode>;<modifier>~
        // Or, if <modifier> is 1 (default):
        // <esc>[<keycode>~
        put_char_to_line_with_val('\033', '^');
        put_char_to_line('[');
        uint8_t keycode = evt.key_code;
        if (keycode >= 100) {
            put_char_to_line('0' + (keycode / 100));
            keycode %= 100;
        }
        if (keycode >= 10) {
            put_char_to_line('0' + (keycode / 10));
            keycode %= 10;
        }
        put_char_to_line('0' + keycode);

        if (modifier != 1) {
            put_char_to_line(';');
            if (modifier >= 10) {
                put_char_to_line('0' + (modifier / 10));
                modifier %= 10;
            }
            put_char_to_line('0' + modifier);
        }

        put_char_to_line('~');
    }
}

static void handle_key_event(keyboard_event_t evt)
{
    if (!evt.pressed) {
        // We only register pressed keys.
        return;
    }

    if (!evt.ascii) {
        handle_non_ascii(evt);
        return;
    }

    if (evt.key_code == KCODE_BACKSPACE) {
        if (current_line_pos > 0) {
            current_line_pos--;
            current_line[current_line_pos] = 0;
            putchar('\b');
        }
    }
    else if (evt.key_code == KCODE_ENTER
        || evt.key_code == KCODE_NUMPAD_ENTER) {
            char nline = '\n';
            put_char_to_line(nline);
            // New line was commited - send it to buffer
            // TODO: if last character was non-escaped backslash, that means
            // "don't commit now, instead pass new line to the reading task"
            push_curr_line_to_buffer();
            if (node != NULL && node->metadata != NULL) {
                tty_state_t* meta = node->metadata;
                if (meta->wait_obj != NULL && meta->wait_obj->count > 0) {
                    wait_wake_up(meta->wait_obj);
                }
            }
    } else {
        put_char_to_line(evt.ascii);
    }
}

static bool is_buffer_ready()
{
    return ready_lines > 0;
}

static size_t read(
    vfs_file_t* file,
    size_t offset __attribute__((unused)),
    size_t size,
    void* ptr
) {
    tty_state_t* meta = file->dentry->inode->metadata;
    wait_on_condition(meta->wait_obj, is_buffer_ready, NULL);
    size_t read_bytes = 0;
    size_t buffer_size = ds_ringbuf_size(buffer);
    if (buffer_size < size) {
        size = buffer_size;
    }

    for (size_t i = 0; i < size; i++) {
        char c;
        ds_ringbuf_pop(buffer, &c);
        *((uint8_t*)ptr + i) = c;
        read_bytes++;
        if (c == '\n') {
            // TODO: if previous character was non-escaped backslash, that means
            // the new line character is a part of line
            ready_lines--;
            break;
        }
    }
    return read_bytes;
}

static void open(
    struct vfs_node* node __attribute__((unused)),
    vfs_file_t* file __attribute__((unused)),
    uint8_t mode __attribute__((unused))
) {
    // Flush current content of TTY when opened
    ds_ringbuf_clear(buffer);
    ready_lines = 0;
    current_line_pos = 0;
    memset(current_line, 0, sizeof(current_line));
}

vfs_node_t* device_tty_mount()
{
    if (node != NULL) {
        return node;
    }

    buffer = ds_ringbuf_create(TTY_BUFFER_SIZE, sizeof(char), true);

    tty_state_t* tty_state = kmalloc(sizeof(tty_state_t));
    tty_state->wait_obj = wait_allocate_queue();

    node = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(node, "tty", VFS_TYPE_CHARACTER_DEVICE);
    node->read_node = read;
    node->open_node = open;
    node->metadata = tty_state;

    keyboard_register_listener(handle_key_event);
    return node;
}

void device_tty_unmount()
{
    if (node == NULL) {
        return;
    }

    if (node->metadata != NULL) {
        tty_state_t* state = node->metadata;
        wait_deallocate(state->wait_obj);
        kfree(node->metadata);
    }
    kfree(node);
    node = NULL;

    if (node != NULL) {
        kfree(node);
        node = NULL;
    }
    if (buffer != NULL) {
        ds_ringbuf_destroy(buffer);
        buffer = NULL;
    }
}
