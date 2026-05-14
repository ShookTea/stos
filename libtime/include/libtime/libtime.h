#ifndef _LIBTIME_H
#define _LIBTIME_H

#include <stdint.h>

/**
 * General library for handling time-related things for kernel, using Jan 1st
 * 2001 at midnight UTC as an epoch.
 */

typedef struct {
    uint16_t year;
    uint8_t month; // 1-12
    uint8_t day; // 1-31
    uint8_t hour; // 0-23
    uint8_t minute; // 0-59
    uint8_t second; // 0-59
    uint16_t millis; // 0-999
} libtime_datetime_t;

#define LIBTIME_DATETIME_ZERO { .year = 0, .month = 0, .day = 0, .hour = 0, .minute = 0, .second = 0, .millis = 0 }
#define LIBTIME_DATETIME_EPOCH { .year = 2001, .month = 1, .day = 1, .hour = 0, .minute = 0, .second = 0, .millis = 0 }

/**
 * Return number of full days since epoch for given date.
 * - `year` - selected year
 * - `month` - month (1-12)
 * - `day` - day (1-31)
 */
int64_t libtime_days_since_epoch(uint16_t year, uint8_t month, uint8_t day);

/**
 * Converts given parameters to timstamp
 */
int64_t libtime_datetime_to_timestamp(libtime_datetime_t* datetime);

/**
 * Read timestamp and store the values in given pointer
 */
void libtime_timestamp_to_datetime(int64_t timestamp, libtime_datetime_t* datetime);

/**
 * Return the weekday number ([0 - 6] -> [Sun - Sat]) for given date.
 */
uint8_t libtime_weekday_from_days(uint16_t year, uint8_t month, uint8_t day);
#endif
