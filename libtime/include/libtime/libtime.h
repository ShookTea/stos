#ifndef _LIBTIME_H
#define _LIBTIME_H

#include <stdint.h>

/**
 * General library for handling time-related things for kernel, using Jan 1st
 * 2001 at midnight UTC as an epoch.
 */

/**
 * Return number of full days since epoch for given date.
 * - `year` - selected year
 * - `month` - month (1-12)
 * - `day` - day (1-31)
 */
int64_t libtime_days_since_epoch(uint16_t year, uint8_t month, uint8_t day);
#endif
