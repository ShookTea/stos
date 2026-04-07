#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>

int exec(const char* path)
{
    return (int)syscall(SYS_EXEC, (int)path, 0, 0);
}

#endif
