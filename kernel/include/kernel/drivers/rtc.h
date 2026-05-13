#ifndef KERNEL_DRIVERS_RTC_H
#define KERNEL_DRIVERS_RTC_H

#include <stdint.h>

/**
 * Initialize RTC driver. It should do everything necessary to make sure that
 * future calls to other functions will work.
 */
void rtc_init(void);

/**
 * Force re-reading current time from RTC chip.
 */
void rtc_resync(void);

/**
 * Return currently stored time as signed int64, measuring (in thousandth of a
 * second) time from midnight, January 1st 2001 UTC.
 */
int64_t rtc_gettime(void);

#endif
