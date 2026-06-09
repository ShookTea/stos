#if !(defined(__is_libk))

#include <sys/stat.h>
#include <fcntl.h>

int mkdir(const char* path, mode_t mode)
{
    return mkdirat(AT_FDCWD, path, mode);
}

#endif
