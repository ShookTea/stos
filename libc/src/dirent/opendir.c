#if !(defined(__is_libk))

#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>

DIR* opendir(const char* name)
{
    int fd = open(name, O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        return NULL;
    }

    DIR* res = malloc(sizeof(DIR));
    res->fd = fd;
    return res;
}

#endif
