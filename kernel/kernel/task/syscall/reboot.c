#include "kernel/task/syscall.h"
#include <sys/reboot.h>

int sys_reboot(int op)
{
    // Implemented in libk
    return reboot(op);
}
