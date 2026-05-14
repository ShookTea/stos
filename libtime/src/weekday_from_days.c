#include <stdint.h>
#include <libtime/libtime.h>

uint8_t libtime_weekday_from_days(uint16_t year, uint8_t month, uint8_t day)
{
    int64_t z = libtime_days_since_epoch(year, month, day);
    return z >= -1 ? ((z + 1) % 7) : (z + 2) % 7 + 6;
}
