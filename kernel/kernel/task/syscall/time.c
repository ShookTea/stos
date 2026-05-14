#include "../syscall.h"
#include <time.h>
#include <stddef.h>

int sys_time(time_t* result_ptr)
{
    if (result_ptr == NULL) {
        return SYSCALL_ERROR;
    }
    // time.h as a libk variant that converts internal time to Unix timestamp
    time(result_ptr);
    return SYSCALL_SUCCESS;
}
