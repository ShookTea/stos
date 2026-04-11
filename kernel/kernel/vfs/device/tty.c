#include "kernel/drivers/keyboard.h"
#include "kernel/task/wait.h"
#include "../device.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "libds/libds.h"
#include "libds/ringbuf.h"
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 4096

static vfs_node_t* node = NULL;
static wait_obj_t* wait_obj;

/**
 * Buffer containing all "committed" bytes (after pressing Enter)
 */
static ds_ringbuf_t* buffer;
static size_t ready_lines = 0;
/**
 * Currently read line
 */
static char current_line[BUFFER_SIZE];
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

static void handle_key_event(keyboard_event_t evt)
{
    if (!evt.pressed || !evt.ascii) {
        // TODO: handle cursor moving left and right
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
            putchar(nline);
            current_line[current_line_pos] = '\n';
            current_line_pos++;
            // New line was commited - send it to buffer
            // TODO: if last character was non-escaped backslash, that means
            // "don't commit now, instead pass new line to the reading task"
            push_curr_line_to_buffer();
            if (wait_obj->count > 0) {
                wait_wake_up(wait_obj);
            }
    } else {
        putchar(evt.ascii);
        current_line[current_line_pos] = evt.ascii;
        current_line_pos++;
    }
}

static bool is_buffer_ready()
{
    return ready_lines > 0;
}

static size_t read(
    vfs_file_t* file __attribute__((unused)),
    size_t offset __attribute__((unused)),
    size_t size,
    void* ptr
) {
    wait_on_condition(wait_obj, is_buffer_ready, NULL);
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

    buffer = ds_ringbuf_create(BUFFER_SIZE, sizeof(char), true);
    node = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(node, "tty", VFS_TYPE_CHARACTER_DEVICE);
    node->read_node = read;
    node->open_node = open;

    wait_obj = wait_allocate_queue();
    keyboard_register_listener(handle_key_event);
    return node;
}

void device_tty_unmount()
{
    if (node != NULL) {
        kfree(node);
        node = NULL;
    }
    if (wait_obj != NULL) {
        wait_deallocate(wait_obj);
        wait_obj = NULL;
    }
    if (buffer != NULL) {
        ds_ringbuf_destroy(buffer);
        buffer = NULL;
    }
}
