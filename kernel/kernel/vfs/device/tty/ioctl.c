#include "kernel/vfs/vfs.h"
#include <stdint.h>
#include "./tty.h"
#include <sys/types.h>
#include <asm/termbits.h>

// This struct should match one in libc/asm/termbits.h
struct termios {
    uint32_t c_iflag;
    uint32_t c_oflag;
    uint32_t c_cflag;
    uint32_t c_lflag;
    unsigned char c_line;
    unsigned char c_cc[10];
};

static int tty_ioctl_tcgets(vfs_file_t* file, struct termios* termios)
{
    uint32_t iflag, oflag, cflag, lflag;
    if (tty_load_termios_flags(file, &iflag, &oflag, &cflag, &lflag) != 0) {
        return -1;
    }
    termios->c_iflag = iflag;
    termios->c_oflag = oflag;
    termios->c_cflag = cflag;
    termios->c_lflag = lflag;

    return 0;
}

static int tty_ioctl_tcsets(vfs_file_t* file, struct termios* termios)
{
    return tty_update_termios_flags(
        file,
        termios->c_iflag,
        termios->c_oflag,
        termios->c_cflag,
        termios->c_lflag
    );
}

static int tty_ioctl_get_fg_pgid(vfs_file_t* file, pid_t* pgid)
{
    tty_state_t* state = file->dentry->inode->metadata;
    *pgid = state->fg_pgid;
    return 0;
}

static int tty_ioctl_set_fg_pgid(vfs_file_t* file, pid_t* pgid)
{
    tty_state_t* state = file->dentry->inode->metadata;
    state->fg_pgid = *pgid;
    return 0;
}

int tty_ioctl(vfs_file_t* file, uint32_t op, void* arg)
{
    if (file == NULL || arg == NULL) {
        return -1;
    }

    switch (op) {
        case TCGETS: {
            struct termios* termios = arg;
            return tty_ioctl_tcgets(file, termios);
        }
        case TCSETS: {
            struct termios* termios = arg;
            return tty_ioctl_tcsets(file, termios);
        }
        case TIOCGPGRP: {
            pid_t* pgid = arg;
            return tty_ioctl_get_fg_pgid(file, pgid);
        }
        case TIOCSPGRP: {
            pid_t* pgid = arg;
            return tty_ioctl_set_fg_pgid(file, pgid);
        }
        default: return -1;
    }
}
