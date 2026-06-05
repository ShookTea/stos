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

/** Read, write, execute/search by owner */
#define S_IRWXU 0x01C0
/** Read permission, owner */
#define S_IRUSR 0x0100
/** Write permission, owner */
#define S_IWUSR 0x0080
/** Execute/search permission, owner */
#define S_IXUSR 0x0040

/** Read, write, execute/search by group */
#define S_IRWXG 0x0038
/** Read permission, group */
#define S_IRGRP 0x0020
/** Write permission, group */
#define S_IWGRP 0x0010
/** Execute/search permission, group */
#define S_IXGRP 0x0008

/** Read, write, execute/search by others */
#define S_IRWXO 0x0007
/** Read permission, others */
#define S_IROTH 0x0004
/** Write permission, others */
#define S_IWOTH 0x0002
/** Execute/search permission, others */
#define S_IXOTH 0x0001

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
