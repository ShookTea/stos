#if !(defined(__is_libk))

#include "./_stdlib_environ.h"

int clearenv(void)
{
    environ = NULL;
    return 0;
}

#endif
