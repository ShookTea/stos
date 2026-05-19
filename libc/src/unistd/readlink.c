#include <unistd.h>
#include <errno.h>

#if !(defined(__is_libk))
    #include <sys/syscall.h>
#else
    #include "kernel/task/task.h"
    #include "kernel/task/scheduler.h"
    #include "kernel/task/syscall.h"
#endif

size_t readlink(const char* restrict path, char* restrict buf, size_t bufsiz)
{
    #if !(defined(__is_libk))

        int res = syscall(SYS_READLINK, (int)path, (int)buf, (int)bufsiz);
        if (res < 0) {
            errno = -res;
            return -1;
        }
        return res;

    #else

        if (bufsiz <= 0) {
            return -EINVAL;
        }

        task_t* current = scheduler_get_current_task();
        if (current == NULL) {
            return -ENOTSUP;
        }

        dentry_t* dentry = vfs_resolve_relative(
            current->root_node,
            current->working_directory,
            path
        );

        if (dentry == NULL) {
            return -ENOENT;
        }

        return vfs_readlink(dentry, buf, bufsiz);

    #endif
}
