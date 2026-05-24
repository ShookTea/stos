#if !(defined(__is_libk))

#include <dirent.h>
#include <unistd.h>

void rewinddir(DIR* dirp)
{
    if (dirp == NULL) {
        return;
    }
    lseek(dirp->fd, 0, SEEK_SET);
}

#endif
