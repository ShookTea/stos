#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <fcntl.h>
#include <stdbool.h>

int open(const char* path, int flags)
{
    int r = (flags & O_RDONLY) ? 1 : 0;
    int w = (flags & O_WRONLY) ? 1 : 0;
    int rw = (flags & O_RDWR) ? 1 : 0;
    if ((r + w + rw) != 1) {
        return -1;
    }

    int res = syscall(SYS_OPEN, (int)path, flags, 0);
    if (res < 3) { // Entries 0-2 are reserved for stdin/out/err
        return -1;
    }
}

#endif
