#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>

int chdir(const char* path)
{
    return syscall(SYS_CHDIR, (int)path, 0, 0);
}

#endif
