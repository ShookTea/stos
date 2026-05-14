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
