#include <sys/reboot.h>
#include <errno.h>

#if defined(__is_libk)
    #include "kernel/acpi.h"
#else
    #include <sys/syscall.h>
#endif


int reboot(int op)
{
    #if !defined(__is_libk)
        int err = syscall(SYS_REBOOT, op, 0, 0);
        if (err < 0) {
            errno = -err;
            return -1;
        }
        return 0;
    #else
        if (op == RB_POWER_OFF) {
            acpi_shutdown();
        }
        else if (op == RB_AUTOBOOT) {
            acpi_reboot();
        }
        return -EINVAL;
    #endif
}
