#include <stddef.h>
#include <time.h>

struct tm* gmtime_r(const time_t* time_ptr, struct tm* tm_ptr)
{
    if (time_ptr == NULL || tm_ptr == NULL) {
        return NULL;
    }
    time_t datetime = *time_ptr;
    time_t time = datetime % (24LL * 60 * 60);

    tm_ptr->tm_sec = time % 60;
    time /= 60;
    tm_ptr->tm_min = time % 60;
    time /= 60;
    tm_ptr->tm_hour = time;

    time_t date = (datetime - time) / (24LL * 60 * 60);
    tm_ptr->tm_wday = date >= -4 ? (date+4) % 7 : (date+5) % 7 + 6;

    date += 719468;
    int era = (date >= 0 ? date : date - 146096) / 146097;
    int doe = (date - era * 146097);
    int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int y = yoe + era * 400;
    int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    int mp = (5 * doy + 2) / 153;
    int d = doy - (153 * mp + 2) / 5 + 1;
    int m = mp < 10 ? mp + 2 : mp - 10;

    tm_ptr->tm_year = (y + (m <= 2 ? 1 : 0)) - 1900;
    tm_ptr->tm_mday = d;
    tm_ptr->tm_mon = m;
    tm_ptr->tm_isdst = -1; // TODO: handle DST

    int is_leap = y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
    static const int days[2][13] = {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
    };
    tm_ptr->tm_yday = days[is_leap][m] + d - 1;

    return tm_ptr;
}
