#ifndef _TIME_H
#define _TIME_H 1

#include <sys/cdefs.h>
#include <stdint.h>

/** Time measurement unit in seconds */
typedef int64_t time_t;

/**
 * Describes time in seconds and nanoseconds.
 */
struct timespec {
    // Full seconds
    time_t tv_spec;
    // Nanoseconds
    long tv_nsec;
};

/**
 * Describes a time in a structured way
 */
struct tm {
    int tm_sec; // seconds [0-61]
    int tm_min; // minutes [0-59]
    int tm_hour; // hour [0-23]
    int tm_mday; // day of month [1-31]
    int tm_mon; // month of year [0-11]
    int tm_year; // years since 1900
    int tm_wday; // day of week [0-6] (Sunday = 0)
    int tm_yday; // day of year [0-365]
    int tm_isdst; // daylight savings flag
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the current time as Unix timestamp - the number of seconds since
 * 1970-01-01 00:00:00 UTC.
 *
 * If `tloc` is not NULL, the return value is stored in the pointer as well.
 */
time_t time(time_t* tloc);

/**
 * Converts Unix timestamp given in `time_ptr` into a broken-down time
 * representation, stores it in `tm_ptr`, and returns `tm_ptr`. It will return
 * `NULL` if either `time_ptr` or `tm_ptr` are `NULL`.
 */
struct tm* gmtime_r(
    const time_t* restrict time_ptr,
    struct tm* restrict tm_ptr
);

/**
 * Converts Unix timestamp given in `time_ptr` into a broken-down time
 * representation and returns pointer to statically allocated struct which might
 * be overwritten by subsequent calls to any of the date and time functions.
 */
struct tm* gmtime(const time_t* time_ptr);

/**
 * Converts Unix timestamp given in `time_ptr` into a broken-down time
 * representation in a local timezone, stores it in `tm_ptr`, and returns
 * `tm_ptr`. It will return `NULL` if either `time_ptr` or `tm_ptr` are `NULL`.
 *
 * TODO: There's no support for local time zones yet, so this function behaves
 * exactly like gmtime_r.
 */
struct tm* localtime_r(
    const time_t* restrict time_ptr,
    struct tm* restrict tm_ptr
);

/**
 * Converts Unix timestamp given in `time_ptr` into a broken-down time
 * representation in a local timezone and returns pointer to statically
 * allocated struct which might be overwritten by subsequent calls to any of the
 * date and time functions.
 *
 * TODO: There's no support for local time zones yet, so this function behaves
 * exactly like gmtime.
 */
struct tm* localtime(const time_t* time_ptr);

/**
 * This function does two primary things:
 * 1. It normalizes value in `tm_ptr` - first by updating structure members that
 *    are outside of their valid interval (for example converting "May 33rd"
 *    to "June 2nd"), and then by updating `tm_yday` and `tm_wday` to correct
 *    values
 * 2. Then it converts given structure into a Unix timestamp that represents
 *    this value.
 *
 * TODO: `tm_isdst` is ignored for now.
 */
time_t mktime(struct tm* tm_ptr);

/**
 * This function takes time from the `tm_ptr` structure and converts it into
 * a string in form:
 *   "Thu May 14 12:58:13 2026\n"
 * and stores it in `buf`, returning pointer to that buffer.
 *
 * Used abbreviations for days of week are: "Sun", "Mon", "Tue", "Wed", "Thu",
 * "Fri", and "Sat".
 * Used abbreviations for months are: "Jan", "Feb", "Mar", "Apr", "May", "Jun",
 * "Jul", "Aug", "Sep", "Oct", "Nov", and "Dec".
 */
char* asctime_r(
    const struct tm* restrict tm_ptr,
    char buf[restrict 26]
);

/**
 * This function takes time from the `tm_ptr` structure and converts it into
 * a string in form:
 *   "Thu May 14 12:58:13 2026\n"
 * and returns pointer to that string. This pointer is statically allocated and
 * its value might be overwritten by subsequent calls to any of the date and
 * time functions.
 *
 * Used abbreviations for days of week are: "Sun", "Mon", "Tue", "Wed", "Thu",
 * "Fri", and "Sat".
 * Used abbreviations for months are: "Jan", "Feb", "Mar", "Apr", "May", "Jun",
 * "Jul", "Aug", "Sep", "Oct", "Nov", and "Dec".
 */
char* asctime(const struct tm* tm_ptr);

/**
 * This function is an equivalent of calling asctime_r(localtime(time_ptr), buf).
 */
char* ctime_r(const time_t* restrict time_ptr, char buf[restrict 26]);

/**
 * This function is equivalent of calling asctime(localtime(time_ptr)).
 */
char* ctime(const time_t* time_ptr);

#if !(defined(__is_libk))

/**
 * Suspends the execution of a calling thread until at least the time specified
 * in `duration` has elapsed.
 *
 * If the call is interrupted by a signal handler, it should return -1,
 * set a correct error code, and store remaining time into `rem` (if not NULL).
 */
int nanosleep(const struct timespec* duration, struct timespec* rem);

#endif // #if !(defined(__is_libk))

#ifdef __cplusplus
}
#endif
#endif
