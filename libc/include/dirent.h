#ifndef _DIRENT_H
#define _DIRENT_H 1

#include <sys/cdefs.h>
#include <sys/types.h>

// Block device
#define DT_BLK 'B'
// Character device
#define DT_CHR 'C'
// Directory
#define DT_DIR 'D'
// Named pipe
#define DT_FIFO 'F'
// Symbolic link
#define DT_LNK 'L'
// Regular file
#define DT_REG 'R'
// UNIX domain socket
#define DT_SOCK 'S'
// Unknown type
#define DT_UNKNOWN 'U'

#define _DIRENT_HAVE_D_TYPE 1

struct dirent {
    ino_t d_ino;
    char d_type;
    char d_name[256];
};

#endif
