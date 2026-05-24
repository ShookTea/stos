#if !(defined(__is_libk))

#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>

int closedir(DIR* dirp)
{
    if (dirp == NULL) {
        errno = EBADF;
        return -1;
    }
    int fd = dirp->fd;
    free(dirp);
    return close(fd);
}

#endif
