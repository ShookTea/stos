#include "kernel/debug.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/task/syscall.h"
#include "kernel/task/wait.h"
#include "../../device.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include <ctype.h>
#include <libds/libds.h>
#include <libds/ringbuf.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "./tty.h"

#define _debug_puts(...) debug_puts_cc(DBC_VFS_DEV, "tty", __VA_ARGS__)
#define _debug_printf(...) debug_printf_cc(DBC_VFS_DEV, "tty", __VA_ARGS__)

static vfs_node_t* node = NULL;

static void push_curr_line_to_buffer(tty_state_t* meta)
{
    for (size_t i = 0; i < meta->current_line_pos; i++) {
        ds_result_t res = ds_ringbuf_push(meta->buffer, &meta->current_line[i]);
        if (res == DS_ERR_FULL) {
            // Clear last character and reattempt pushing. We're doing it
            // manually instead of passing fail_on_full=false to the ringbuf
            // creation, because we need to decrent ready_lines when hitting
            // new line.
            char popped;
            ds_ringbuf_pop(meta->buffer, &popped);
            if (popped == '\n') {
                meta->ready_lines--;
            }
            // Re-attempt pushing
            ds_ringbuf_push(meta->buffer, &meta->current_line[i]);
        }
    }
    meta->current_line_pos = 0;
    meta->ready_lines++;
}

static inline void put_char_to_line_with_val(
    tty_state_t* meta,
    char val,
    char* to_print
) {
    if (meta->lflag & TTY_LFLAG_ICANON) {
        if (meta->current_line_pos < TTY_BUFFER_SIZE) {
            if (meta->lflag & TTY_LFLAG_ECHO) {
                while (*to_print != '\0') putchar(*to_print++);
            }
            meta->current_line[meta->current_line_pos] = val;
            meta->current_line_pos++;
        }
    }
    else {
        if (meta->lflag & TTY_LFLAG_ECHO) {
            while (*to_print != '\0') putchar(*to_print++);
        }
        ds_ringbuf_push(meta->buffer, &val);
        if (meta->wait_obj != NULL && meta->wait_obj->count > 0) {
            wait_wake_up(meta->wait_obj);
        }
    }
}

static inline void put_char_to_line(tty_state_t* meta, char c)
{
    char to_print[2] = "_";
    to_print[0] = c;
    put_char_to_line_with_val(meta, c, to_print);
}

static void handle_non_ascii(tty_state_t* meta, keyboard_event_t evt)
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

    _debug_printf("keycode = %u, mod = %u\n", evt.key_code, modifier);

    if (evt.key_code == KCODE_ARROW_LEFT
        || evt.key_code == KCODE_ARROW_RIGHT
        || evt.key_code == KCODE_ARROW_UP
        || evt.key_code == KCODE_ARROW_DOWN
        || evt.key_code == KCODE_HOME
        || evt.key_code == KCODE_END
    ) {
        // Special case for events with no modifiers added
        put_char_to_line_with_val(meta, '\033', "^");
        put_char_to_line(meta, '[');

        if (modifier != 1) {
            if (modifier >= 10) {
                put_char_to_line(meta, '0' + (modifier / 10));
                modifier %= 10;
            }
            put_char_to_line(meta, '0' + modifier);
        }

        switch (evt.key_code) {
            case KCODE_ARROW_UP: put_char_to_line(meta, 'A'); break;
            case KCODE_ARROW_DOWN: put_char_to_line(meta, 'B'); break;
            case KCODE_ARROW_RIGHT: put_char_to_line(meta, 'C'); break;
            case KCODE_ARROW_LEFT: put_char_to_line(meta, 'D'); break;
            case KCODE_HOME: put_char_to_line(meta, 'H'); break;
            case KCODE_END: put_char_to_line(meta, 'F'); break;
        }
    }
    else {
        // Send key in format <esc>[<keycode>;<modifier>~
        // Or, if <modifier> is 1 (default):
        // <esc>[<keycode>~
        put_char_to_line_with_val(meta, '\033', "^");
        put_char_to_line(meta, '[');
        uint8_t keycode = evt.key_code;
        if (keycode >= 100) {
            put_char_to_line(meta, '0' + (keycode / 100));
            keycode %= 100;
        }
        if (keycode >= 10) {
            put_char_to_line(meta, '0' + (keycode / 10));
            keycode %= 10;
        }
        put_char_to_line(meta, '0' + keycode);

        if (modifier != 1) {
            put_char_to_line(meta, ';');
            if (modifier >= 10) {
                put_char_to_line(meta, '0' + (modifier / 10));
                modifier %= 10;
            }
            put_char_to_line(meta, '0' + modifier);
        }

        put_char_to_line(meta, '~');
    }
}

void tty_push_char_to_buffer(char c)
{
    if (node == NULL || node->metadata == NULL) {
        return;
    }
    tty_state_t* meta = node->metadata;
    put_char_to_line(meta, c);
}

static void handle_ascii_with_ctrl(char ascii)
{
    tty_state_t* meta = node->metadata;
    if (ascii == 'c') {
        sys_sigsend(-meta->fg_pgid, SIGINT);
    }
    else if (ascii == 'z') {
        sys_sigsend(-meta->fg_pgid, SIGTSTP);
    }

    if (isalpha(ascii)) {
        // First, convert the ASCII to the uppercase
        if (islower(ascii)) {
            ascii -= 32;
        }
        // Then convert it to a special character
        // ('A' -> '^A' etc)
        char spec_char = ascii - 64;
        char to_print[3] = "^_";
        to_print[1] = ascii;
        // Emit the character:
        put_char_to_line_with_val(meta, spec_char, to_print);
    }
}

static void handle_key_event(keyboard_event_t evt)
{
    if (node == NULL || node->metadata == NULL) {
        return;
    }
    tty_state_t* meta = node->metadata;

    if (!evt.pressed) {
        // We only register pressed keys.
        return;
    }

    if (!evt.ascii) {
        handle_non_ascii(meta, evt);
        return;
    }

    bool ctrl = evt.l_ctrl_pressed || evt.r_ctrl_pressed;
    if (ctrl) {
        handle_ascii_with_ctrl(evt.ascii);
        return;
    }

    if (evt.l_ctrl_pressed && evt.ascii == 'c') {
        sys_sigsend(-meta->fg_pgid, SIGINT);
        return;
    }
    if (evt.l_ctrl_pressed && evt.ascii == 'z') {
        sys_sigsend(-meta->fg_pgid, SIGTSTP);
        return;
    }

    if (evt.key_code == KCODE_BACKSPACE && (meta->lflag & TTY_LFLAG_ICANON)) {
        if (meta->current_line_pos > 0) {
            meta->current_line_pos--;
            meta->current_line[meta->current_line_pos] = 0;
            putchar('\b');
        }
    }
    else if ((evt.key_code == KCODE_ENTER
        || evt.key_code == KCODE_NUMPAD_ENTER)
        && meta->lflag & TTY_LFLAG_ICANON) {
            tty_push_char_to_buffer('\n');
            // New line was commited - send it to buffer
            // TODO: if last character was non-escaped backslash, that means
            // "don't commit now, instead pass new line to the reading task"
            push_curr_line_to_buffer(meta);
            if (meta->wait_obj != NULL && meta->wait_obj->count > 0) {
                wait_wake_up(meta->wait_obj);
            }
    } else {
        tty_push_char_to_buffer(evt.ascii);
    }
}

static int open(
    struct vfs_node* node,
    vfs_file_t* file __attribute__((unused)),
    uint8_t mode __attribute__((unused))
) {
    tty_state_t* meta = node->metadata;
    // Flush current content of TTY when opened
    ds_ringbuf_clear(meta->buffer);
    meta->ready_lines = 0;
    meta->current_line_pos = 0;
    memset(meta->current_line, 0, sizeof(meta->current_line));
    return 0;
}

vfs_node_t* device_tty_mount()
{
    if (node != NULL) {
        return node;
    }

    tty_state_t* tty_state = kmalloc(sizeof(tty_state_t));
    tty_state->wait_obj = wait_allocate_queue();
    tty_state->buffer = ds_ringbuf_create(TTY_BUFFER_SIZE, sizeof(char), true);
    tty_state->ready_lines = 0;
    tty_state->iflag = TTY_DEFAULT_IFLAG;
    tty_state->oflag = TTY_DEFAULT_OFLAG;
    tty_state->cflag = TTY_DEFAULT_CFLAG;
    tty_state->lflag = TTY_DEFAULT_LFLAG;
    tty_state->fg_pgid = 0;

    node = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(node, "tty", VFS_TYPE_CHARACTER_DEVICE);
    node->read_node = tty_read;
    node->write_node = tty_write;
    node->open_node = open;
    node->ioctl_node = tty_ioctl;
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
        ds_ringbuf_destroy(state->buffer);
        kfree(node->metadata);
    }
    kfree(node);
    node = NULL;
}
