#include <time.h>
#include <stddef.h>
#include "./_time.h"

static inline int floor_div(int a, int b) {
    return a / b - (a % b != 0 && (a ^ b) < 0);
}

static inline int floor_mod(int a, int b) {
    return a - floor_div(a, b) * b;
}

static inline int is_leap_year(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static int days_in_month(int mon, int year) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (mon == 1 && is_leap_year(year)) return 29;
    return days[mon];
}

static int day_of_week(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 2) y--;
    return (y + y/4 - y/100 + y/400 + t[m] + d) % 7;
}

void __time_normalize(struct tm* tm)
{
    if (tm == NULL) {
        return;
    }

    tm->tm_min += floor_div(tm->tm_sec, 60);
    tm->tm_sec = floor_mod(tm->tm_sec, 60);

    tm->tm_hour += floor_div(tm->tm_min, 60);
    tm->tm_min = floor_mod(tm->tm_min, 60);

    tm->tm_mday += floor_div(tm->tm_hour, 24);
    tm->tm_hour = floor_mod(tm->tm_hour, 24);

    // Normalize months before days so leap year calculation is correct
    tm->tm_year += floor_div(tm->tm_mon, 12);
    tm->tm_mon = floor_mod(tm->tm_mon, 12);

    while (tm->tm_mday < 1) {
        tm->tm_mon--;
        if (tm->tm_mon < 0) {
            tm->tm_mon = 11;
            tm->tm_year--;
        }
        tm->tm_mday += days_in_month(tm->tm_mon, tm->tm_year + 1900);
    }
    while (tm->tm_mday > days_in_month(tm->tm_mon, tm->tm_year + 1900)) {
        tm->tm_mday -= days_in_month(tm->tm_mon, tm->tm_year + 1900);
        tm->tm_mon++;
        if (tm->tm_mon > 11) {
            tm->tm_mon = 0;
            tm->tm_year++;
        }
    }

    static const int yday_offsets[2][12] = {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
    };
    int leap = is_leap_year(tm->tm_year + 1900);
    tm->tm_yday = yday_offsets[leap][tm->tm_mon] + tm->tm_mday - 1;
    tm->tm_wday = day_of_week(tm->tm_year + 1900, tm->tm_mon, tm->tm_mday);
}
