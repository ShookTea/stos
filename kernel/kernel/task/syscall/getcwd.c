#include "kernel/task/syscall.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/vfs/path.h"
#include <stdint.h>

char* sys_getcwd(char* buf, size_t size)
{
    task_t* curr = scheduler_get_current_task();
    if (curr == NULL) {
        return NULL;
    }

    return path_build_absolute(
        curr->root_node,
        curr->working_directory,
        buf,
        size
    );
}
