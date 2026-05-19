#include <time.h>
#include <stddef.h>
#include <errno.h>
#include "kernel/task/syscall.h"

int sys_time(time_t* result_ptr)
{
    if (result_ptr == NULL) {
        return -EFAULT;
    }
    // time.h as a libk variant that converts internal time to Unix timestamp
    time(result_ptr);
    return 0;
}
