#include <stddef.h>
#include <stdio.h>
#include <time.h>

static char* weekday[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static char* month[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char* asctime_r(
    const struct tm* restrict tm_ptr,
    char buf[restrict 26]
) {
    if (
        tm_ptr == NULL
        || tm_ptr->tm_mon < 0 || tm_ptr->tm_mon >= 12
        || tm_ptr->tm_wday < 0 || tm_ptr->tm_wday >= 7
    ) {
        return NULL;
    }
    sprintf(
        buf,
        "%s %s %02u %02u:%02u:%02u %04u\n",
        weekday[tm_ptr->tm_wday],
        month[tm_ptr->tm_mon],
        tm_ptr->tm_mday,
        tm_ptr->tm_hour,
        tm_ptr->tm_min,
        tm_ptr->tm_sec,
        tm_ptr->tm_year + 1900
    );

    return buf;
}
