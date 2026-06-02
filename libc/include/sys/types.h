#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H 1

#include <sys/cdefs.h>
#include <stdint.h>
#include <stddef.h>

typedef size_t   blkcnt_t;
typedef size_t   blksize_t;
typedef uint32_t dev_t;
typedef uint16_t gid_t;
typedef uint32_t ino_t;
typedef uint16_t mode_t;
typedef uint16_t nlink_t;
typedef size_t   off_t;
typedef uint16_t uid_t;
typedef uint32_t pid_t;

#ifndef _SSIZE_T_DEFINED_
#define _SSIZE_T_DEFINED_
    typedef int64_t ssize_t;
#endif

#endif
