#include <time.h>
#include <stddef.h>
#include "./_time.h"

static time_t days_since_epoch(int y, int m, int d)
{
    m++; // convert from [0-11] to [1-12]
    y -= m <= 2;
    int era = (y >= 0 ? y : y-399) / 400;
    int yoe = y - era * 400;
    int doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

time_t mktime(struct tm* tm_ptr)
{
    if (tm_ptr == NULL) {
        return 0;
    }

    __time_normalize(tm_ptr);
    time_t res = days_since_epoch(
        tm_ptr->tm_year + 1900,
        tm_ptr->tm_mon,
        tm_ptr->tm_mday
    );
    res = (res * 24) + tm_ptr->tm_hour;
    res = (res * 60) + tm_ptr->tm_min;
    res = (res * 60) + tm_ptr->tm_sec;

    return res;
}
