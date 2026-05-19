#include <unistd.h>
#include "kernel/task/syscall.h"

int sys_readlink(const char* path, char* buf, int bufsiz)
{
    // implemented in libk
    return readlink(path, buf, bufsiz);
}
