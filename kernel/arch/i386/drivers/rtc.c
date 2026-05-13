#include <stdint.h>
#include <stdbool.h>
#include "../io.h"
#include <libtime/libtime.h>
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c(DBC_RTC, __VA_ARGS__)
#define _debug_printf(...) debug_printf_c(DBC_RTC, __VA_ARGS__)

#define CMOS_SELECTOR_PORT 0x70
#define CMOS_DATA_PORT 0x71
#define CMOS_REGISTRY_NO_NMI_BIT (1 << 7)

// This should be updated every year. No drama if it won't though - it's safe
// to be used for full 100 years.
#define COMPILATION_YEAR 2026

#define CMOS_REG_SECONDS 0x00
#define CMOS_REG_MINUTES 0x02
#define CMOS_REG_HOURS 0x04
#define CMOS_REG_DAY 0x07
#define CMOS_REG_MONTH 0x08
#define CMOS_REG_YEAR 0x09
#define CMOS_REG_STATUS_A 0x0A
#define CMOS_REG_STATUS_B 0x0B

#define BCD_TO_BIN(v) (((v & 0xF0) >> 1) + ((v & 0xF0) >> 3) + (v & 0xF))

static uint8_t century_register = 0;
static int64_t curr_time = 0;

static inline uint8_t read_byte(uint8_t registry)
{
    outb(CMOS_SELECTOR_PORT, CMOS_REGISTRY_NO_NMI_BIT | registry);
    return inb(CMOS_DATA_PORT);
}

static bool cmos_update_in_progress()
{
    return (read_byte(CMOS_REG_STATUS_A) & 0x80);
}

void rtc_init(void)
{
    // TODO: read form ACPI if century register is present
    // TODO: setup PIT
}

void rtc_resync(void)
{
    _debug_puts("RTC resync requested");
    uint8_t second, prev_second, minute, prev_minute, hour, prev_hour,
            day, prev_day, month, prev_month,
            century = 0, prev_century = 0, status_b;
    uint16_t year, prev_year;
    while (cmos_update_in_progress()); // Wait for update to be completed

    // Read first batch of data
    second = read_byte(CMOS_REG_SECONDS);
    minute = read_byte(CMOS_REG_MINUTES);
    hour = read_byte(CMOS_REG_HOURS);
    day = read_byte(CMOS_REG_DAY);
    month = read_byte(CMOS_REG_MONTH);
    year = read_byte(CMOS_REG_YEAR);
    if (century_register != 0) {
        century = read_byte(century_register);
    }

    // Repeat reading until there are no changes between previous and new values
    do {
        prev_second = second;
        prev_minute = minute;
        prev_hour = hour;
        prev_day = day;
        prev_month = month;
        prev_year = year;
        prev_century = century;

        while (cmos_update_in_progress());

        second = read_byte(CMOS_REG_SECONDS);
        minute = read_byte(CMOS_REG_MINUTES);
        hour = read_byte(CMOS_REG_HOURS);
        day = read_byte(CMOS_REG_DAY);
        month = read_byte(CMOS_REG_MONTH);
        year = read_byte(CMOS_REG_YEAR);
        if (century_register != 0) {
            century = read_byte(century_register);
        }
    } while ((second != prev_second) || (minute != prev_minute)
        || (hour != prev_hour) || (day != prev_day)
        || (month != prev_month) || (year != prev_year)
        || (century != prev_century));

    status_b = read_byte(CMOS_REG_STATUS_B);

    // If in BCD format, convert to normal binary
    if (!(status_b & 0x04)) {
        year = BCD_TO_BIN(year);
        month = BCD_TO_BIN(month);
        day = BCD_TO_BIN(day);
        // Separate conversion to handle both 12-hour and 24-hour clock
        hour = ((hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
        minute = BCD_TO_BIN(minute);
        second = BCD_TO_BIN(second);
    }

    // If in 12-hour format, convert to 24-hour
    if (!(status_b & 0x02) && (hour & 0x80)) {
          hour = ((hour & 0x7F) + 12) % 24;
    }

    if (century_register != 0) {
          year += century * 100;
    } else {
          year += COMPILATION_YEAR / 100 * 100;
          if( year < COMPILATION_YEAR) {
              year += 100;
          }
    }

    _debug_printf(
        "Value read: %04u-%02u-%02u %02u:%02u:%02u (0x%02X)\n",
        year, month, day,
        hour, minute, second,
        status_b
    );

    libtime_datetime_t parts = LIBTIME_DATETIME_ZERO;
    parts.year = year;
    parts.month = month;
    parts.day = day;
    parts.hour = hour;
    parts.minute = minute;
    parts.second = second;

    curr_time = libtime_datetime_to_timestamp(&parts);
}

int64_t rtc_gettime(void)
{
    return curr_time;
}
