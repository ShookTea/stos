#include <libtime/libtime.h>
#include <stddef.h>
#include <stdint.h>

int64_t libtime_datetime_to_timestamp(
    libtime_datetime_t* dt
) {
    if (dt == NULL) {
        return 0;
    }
    int64_t res = libtime_days_since_epoch(dt->year, dt->month, dt->day);
    res = (res * 24) + dt->hour;
    res = (res * 60) + dt->minute;
    res = (res * 60) + dt->second;
    res = (res * 1000) + dt->millis;
    return res;
}
