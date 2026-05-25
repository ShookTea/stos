#include <errno.h>
#include "kernel/task/syscall.h"

int sys_pipe(int pipefd[2] __attribute__((unused)), int flags __attribute__((unused)))
{
    // TODO: implementation of the syscall
    return -ENOTSUP;
}
