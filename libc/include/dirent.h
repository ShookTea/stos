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

#if !(defined(__is_libk))

/**
 * Directory stream structure
 */
typedef struct {
    /** File descriptor ID (from fcntl.h) */
    int fd;
} DIR;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens a directory and returns directory stream on success. On failure returns
 * NULL and sets `errno` to one of error values as described by `open()`.
 */
DIR* opendir(const char* name);

/**
 * Closes directory stream and returns 0 on success. On failure returns -1
 * and sets `errno` to one of following values:
 * - EBADF - invalid directory stream descriptor
 */
int closedir(DIR* dirp);

/**
 * Returns a pointer to a `dirent` structure representing the next directory
 * entry in directory `dirp`. It returns `NULL` on reaching the end of the
 * directory entry or if error occured (in which case `errno` will be set as
 * well).
 *
 * The returned data may be overwritten by subsequent calls to this function.
 */
struct dirent* readdir(DIR* dirp);

/**
 * Resets position of the directory stream `dirp` to the beginning of the
 * stream.
 */
void rewinddir(DIR* dirp);

#ifdef __cplusplus
}
#endif

#endif

#endif
