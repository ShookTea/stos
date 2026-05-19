#include "kernel/task/syscall.h"
#include <errno.h>
#include "kernel/task/scheduler.h"
#include "kernel/task/wait.h"

// TODO: If the call is interrupted by a signal handler, it should return -1,
// set a correct error code, and store remaining time into `rem` (if not NULL).
int sys_sleep(
    const struct timespec* duration,
    struct timespec* rem __attribute__((unused))
) {
    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return -ENOTSUP;
    }

    wait_sleep((duration->tv_nsec / 1000) + (duration->tv_spec * 1000));
    return 0;
}
