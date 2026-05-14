#include <stddef.h>
#include <time.h>

#if (defined(__is_libk))
    #include "kernel/drivers/rtc.h"
    #include <libtime/libtime.h>
#else
    #include <sys/syscall.h>
#endif

time_t time(time_t* tloc)
{
    time_t res = 0;

    #if (defined(__is_libk))
        // Milliseconds since 2001-01-01 00:00:00 UTC
        int64_t rtc_time = rtc_gettime();
        res = libtime_timestamp_to_unix(rtc_time);
    #else
        syscall(SYS_UNIXTIME, (int)&res, 0, 0);
    #endif

    if (tloc != NULL) {
        *tloc = res;
    }
    return res;

}
