#include <libtime/libtime.h>
#include <stdint.h>

int64_t libtime_days_since_epoch(uint16_t year, uint8_t month, uint8_t day)
{
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    int64_t era = year / 400;
    int64_t yoe = year - era * 400;
    int64_t doy = (153 * (month - 3) + 2)/5 + day - 1;
    int64_t doe = yoe * 365 + yoe/4 - yoe/100 + yoe/400 + doy;
    return era * 146097 + doe - 730791;
}
