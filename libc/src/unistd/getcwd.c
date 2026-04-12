#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>

char* getcwd(char buf[], size_t size)
{
    return (char*)syscall(SYS_GETCWD, (int)buf, (int)size, 0);
}

#endif
