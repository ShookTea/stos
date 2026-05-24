#include <errno.h>
#if !(defined(__is_libk))

#include <dirent.h>
#include <unistd.h>
#include <sys/syscall.h>

struct dirent* readdir(DIR* dirp)
{
    static struct dirent dir;

    if (dirp == NULL) {
        errno = EBADF;
        return NULL;
    }

    int res = syscall(SYS_GETDENTS, dirp->fd, (int)&dir, 1);
    if (res < 0) {
        errno = -res;
    }
    if (res <= 0) {
        return NULL;
    }
    return &dir;
}

#endif
