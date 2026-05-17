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

#endif
