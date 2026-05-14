#include <libtime/libtime.h>
#include <stddef.h>
#include <stdint.h>

void libtime_timestamp_to_datetime(int64_t ts, libtime_datetime_t* dt)
{
    if (dt == NULL) {
        return;
    }
    int64_t ms = ts % (24LL * 60 * 60 * 1000);
    if (ms < 0) ms += 24LL * 60 * 60 * 1000;
    ts = (ts - ms) / (24LL * 60 * 60 * 1000);

    dt->millis = ms % 1000; ms /= 1000;
    dt->second = ms % 60;   ms /= 60;
    dt->minute = ms % 60;   ms /= 60;
    dt->hour   = ms;

    ts += 730791;
    int era = (ts >= 0 ? ts : ts - 146096) / 146097;
    int doe = ts - era * 146097;
    int yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    int y = yoe + era * 400;
    int doy = doe - (365 * yoe + yoe/4 - yoe/100);
    int mp = (5 * doy + 2)/153;
    int d = doy - (153 *mp + 2)/5 + 1;
    int m = mp < 10 ? mp + 3 : mp - 9;
    dt->year = y + (m <= 2 ? 1 : 0);
    dt->month = m;
    dt->day = d;
}
