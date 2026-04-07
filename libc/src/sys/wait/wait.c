#if !(defined(__is_libk))

#include <sys/wait.h>

int wait(int* status_code)
{
    return waitpid(-1, status_code, 0);
}

#endif
