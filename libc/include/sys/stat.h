#ifndef _SYS_STAT_H
#define _SYS_STAT_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stddef.h>
#include <time.h>

struct stat {
    dev_t st_dev; // ID of device containing file
    ino_t st_ino; // Inode number
    mode_t st_mode; // File type and mode
    nlink_t st_nlink; // Number of hard links
    uid_t st_uid; // User ID of owner
    gid_t st_gid; // Group ID of owner
    dev_t st_rdev; // device ID (if special file)
    off_t st_size; // Total size, in bytes
    blksize_t st_blksize; // Block size for filesystem I/O
    blkcnt_t st_blocks; // Number of 512 B blocks allocated
    struct timespec st_atim; // Time of last access
    struct timespec st_mtim; // Time of last modification
    struct timespec st_ctim; // Creation time
};

#ifdef __cplusplus
extern "C" {
#endif

#if !(defined(__is_libk))

/**
 * Loads stats about given `path` and stores them in `statbuf`. On success
 * returns 0, on failure returns -1 and sets `errno` to a value.
 */
int stat(const char* restrict path, struct stat* restrict statbuf);

/**
 * Loads stats about file described in file descriptor `fd` and stores them
 * in `statbuf`. On success returns 0, on failure returns -1 and sets `errno`
 * to a value.
 */
int fstat(int fd, struct stat* statbuf);

/**
 * Behaves similarly to `stat`, but if `path` points to a soft link file, it
 * will use statistics about that file instead of the one pointed to by the
 * soft link.
 */
int lstat(const char* restrict path, struct stat* restrict statbuf);

#endif

#ifdef __cplusplus
}
#endif

#endif
