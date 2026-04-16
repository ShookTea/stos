#include <stdint.h>
#include "kernel/vfs/vfs.h"
#include "./tty.h"

int tty_update_termios_flags(
    vfs_file_t* tty,
    uint32_t iflag,
    uint32_t oflag,
    uint32_t cflag,
    uint32_t lflag
) {
    if (tty == NULL) {
        return -1;
    }

    tty_state_t* meta = tty->dentry->inode->metadata;
    meta->iflag = iflag;
    meta->oflag = oflag;
    meta->cflag = cflag;
    meta->lflag = lflag;

    return 0;
}

int tty_load_termios_flags(
    vfs_file_t* tty,
    uint32_t* iflag,
    uint32_t* oflag,
    uint32_t* cflag,
    uint32_t* lflag
) {
    if (tty == NULL) {
        return -1;
    }

    tty_state_t* meta = tty->dentry->inode->metadata;
    if (iflag != NULL) *iflag = meta->iflag;
    if (oflag != NULL) *oflag = meta->oflag;
    if (cflag != NULL) *cflag = meta->cflag;
    if (lflag != NULL) *lflag = meta->lflag;

    return 0;
}
