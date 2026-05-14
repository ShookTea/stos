#include "_time.h"
#include <stddef.h>
#include <time.h>

struct tm* gmtime_r(const time_t* time_ptr, struct tm* tm_ptr)
{
    if (time_ptr == NULL || tm_ptr == NULL) {
        return NULL;
    }
    time_t datetime = *time_ptr;
    time_t time = datetime % (24LL * 60 * 60);
    if (time < 0) time += (24LL * 60 * 60);
    time_t date = (datetime - time) / (24LL * 60 * 60);

    tm_ptr->tm_sec = time;
    tm_ptr->tm_mday = 1 + date;
    // Setup empty values for normalization
    tm_ptr->tm_min = 0;
    tm_ptr->tm_hour = 0;
    tm_ptr->tm_wday = 0;
    tm_ptr->tm_mon = 0;
    tm_ptr->tm_year = 70;

    __time_normalize(tm_ptr);

    tm_ptr->tm_isdst = -1; // TODO: handle DST

    return tm_ptr;
}
