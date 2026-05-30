#include "kernel/task/syscall.h"
#include <errno.h>
#include <time.h>
#include "kernel/task/scheduler.h"
#include "kernel/task/wait.h"

int sys_sleep(const struct timespec* duration, struct timespec* rem)
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return -ENOTSUP;
    }

    size_t millis =
        (size_t)(duration->tv_spec * 1000)
        + (size_t)(duration->tv_nsec / 1000000);
    size_t remaining_ms = wait_sleep(millis);

    if (remaining_ms > 0) {
        if (rem != NULL) {
            rem->tv_spec = (time_t)(remaining_ms / 1000);
            rem->tv_nsec = (long)((remaining_ms % 1000) * 1000000);
        }
        return -EINTR;
    }
    return 0;
}
