#ifndef INCLUDE_KERNEL_SRC_VFS_DEVICE_TTY_H
#define INCLUDE_KERNEL_SRC_VFS_DEVICE_TTY_H

#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include <libds/ringbuf.h>
#include <stdint.h>

#define TTY_BUFFER_SIZE 4096

// Should typed characters be displayed in stdout as well?
#define TTY_LFLAG_ECHO 0x01
// Running in canonical (line-buffered) mode
#define TTY_LFLAG_ICANON 0x02

#define TTY_DEFAULT_IFLAG 0
#define TTY_DEFAULT_OFLAG 0
#define TTY_DEFAULT_CFLAG 0
#define TTY_DEFAULT_LFLAG (TTY_LFLAG_ECHO | TTY_LFLAG_ICANON)

typedef struct {
    // termios input mode flags
    uint32_t iflag;
    // termios output mode flags
    uint32_t oflag;
    // termios control mode flags
    uint32_t cflag;
    // termios local mode flags
    uint32_t lflag;
    // Queue of processes waiting for read from the TTY
    wait_obj_t* wait_obj;
    // Buffer of all "commited" bytes that are ready to read
    ds_ringbuf_t* buffer;
    size_t ready_lines;
    // Currently read line
    char current_line[TTY_BUFFER_SIZE];
    size_t current_line_pos;
} tty_state_t;

/**
 * Update selected TTY file with termios flags. Returns 0 on success.
 */
int tty_update_termios_flags(
    vfs_file_t* tty,
    uint32_t iflag,
    uint32_t oflag,
    uint32_t cflag,
    uint32_t lflag
);

#endif
