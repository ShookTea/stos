#include <errno.h>
#include "kernel/vfs/pipe.h"
#include "kernel/task/syscall.h"

int sys_pipe(int pipefd[2], int flags)
{
    if (pipefd == NULL) {
        return -EFAULT;
    }
    return pipe_create(pipefd, pipefd + 1, flags);
}
