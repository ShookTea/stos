#ifndef KERNEL_LIBC_TIME_TESTS_H
#define KERNEL_LIBC_TIME_TESTS_H

#include <time.h>
#include <stdint.h>
#include "test_base.h"
#include "kernel/debug.h"

static inline bool time_test_gmtime_epoch(void) {
    struct tm tm;
    time_t ts = 0;
    gmtime_r(&ts, &tm);
    ASSERT_EQ(tm.tm_sec, 0, "gmtime_epoch tm_sec");
    ASSERT_EQ(tm.tm_min, 0, "gmtime_epoch tm_min");
    ASSERT_EQ(tm.tm_hour, 0, "gmtime_epoch tm_hour");
    ASSERT_EQ(tm.tm_mday, 1, "gmtime_epoch tm_mday");
    ASSERT_EQ(tm.tm_mon, 0, "gmtime_epoch tm_mon");
    ASSERT_EQ(tm.tm_year, 1970 - 1900, "gmtime_epoch tm_year");
    ASSERT_EQ(tm.tm_wday, 4, "gmtime_epoch tm_wday");
    ASSERT_EQ(tm.tm_yday, 0, "gmtime_epoch tm_yday");
    return true;
}

static inline bool time_test_gmtime_future1(void) {
    struct tm tm;
    time_t ts = 1778748085LL; // 2026-05-14
    gmtime_r(&ts, &tm);
    ASSERT_EQ(tm.tm_sec, 25, "gmtime_epoch tm_sec");
    ASSERT_EQ(tm.tm_min, 41, "gmtime_epoch tm_min");
    ASSERT_EQ(tm.tm_hour, 8, "gmtime_epoch tm_hour");
    ASSERT_EQ(tm.tm_mday, 14, "gmtime_epoch tm_mday");
    ASSERT_EQ(tm.tm_mon, 4, "gmtime_epoch tm_mon");
    ASSERT_EQ(tm.tm_year, 2026 - 1900, "gmtime_epoch tm_year");
    ASSERT_EQ(tm.tm_wday, 4, "gmtime_epoch tm_wday");
    ASSERT_EQ(tm.tm_yday, 133, "gmtime_epoch tm_yday");
    return true;
}

static inline bool time_test_gmtime_future2(void) {
    struct tm tm;
    time_t ts = 5299480968LL; // 2137-12-07
    gmtime_r(&ts, &tm);
    ASSERT_EQ(tm.tm_sec, 48, "gmtime_epoch tm_sec");
    ASSERT_EQ(tm.tm_min, 2, "gmtime_epoch tm_min");
    ASSERT_EQ(tm.tm_hour, 14, "gmtime_epoch tm_hour");
    ASSERT_EQ(tm.tm_mday, 7, "gmtime_epoch tm_mday");
    ASSERT_EQ(tm.tm_mon, 11, "gmtime_epoch tm_mon");
    ASSERT_EQ(tm.tm_year, 2137 - 1900, "gmtime_epoch tm_year");
    ASSERT_EQ(tm.tm_wday, 6, "gmtime_epoch tm_wday");
    ASSERT_EQ(tm.tm_yday, 340, "gmtime_epoch tm_yday");
    return true;
}

static inline bool time_test_gmtime_leapyear1(void) {
    struct tm tm;
    time_t ts = 1468414968LL; // 2016-07-13 (handle leap year)
    gmtime_r(&ts, &tm);
    ASSERT_EQ(tm.tm_sec, 48, "gmtime_epoch tm_sec");
    ASSERT_EQ(tm.tm_min, 2, "gmtime_epoch tm_min");
    ASSERT_EQ(tm.tm_hour, 13, "gmtime_epoch tm_hour");
    ASSERT_EQ(tm.tm_mday, 13, "gmtime_epoch tm_mday");
    ASSERT_EQ(tm.tm_mon, 6, "gmtime_epoch tm_mon");
    ASSERT_EQ(tm.tm_year, 2016 - 1900, "gmtime_epoch tm_year");
    ASSERT_EQ(tm.tm_wday, 3, "gmtime_epoch tm_wday");
    ASSERT_EQ(tm.tm_yday, 194, "gmtime_epoch tm_yday");
    return true;
}

static inline bool time_test_gmtime_leapyear2(void) {
    struct tm tm;
    time_t ts = 4119166968LL; // 2100-07-13 (not a leap year)
    gmtime_r(&ts, &tm);
    ASSERT_EQ(tm.tm_sec, 48, "gmtime_epoch tm_sec");
    ASSERT_EQ(tm.tm_min, 2, "gmtime_epoch tm_min");
    ASSERT_EQ(tm.tm_hour, 13, "gmtime_epoch tm_hour");
    ASSERT_EQ(tm.tm_mday, 13, "gmtime_epoch tm_mday");
    ASSERT_EQ(tm.tm_mon, 6, "gmtime_epoch tm_mon");
    ASSERT_EQ(tm.tm_year, 2100 - 1900, "gmtime_epoch tm_year");
    ASSERT_EQ(tm.tm_wday, 2, "gmtime_epoch tm_wday");
    ASSERT_EQ(tm.tm_yday, 193, "gmtime_epoch tm_yday");
    return true;
}

static inline bool time_test_gmtime_leapyear3(void) {
    struct tm tm;
    time_t ts = 963493368LL; // 2000-07-13 (a leap year)
    gmtime_r(&ts, &tm);
    ASSERT_EQ(tm.tm_sec, 48, "gmtime_epoch tm_sec");
    ASSERT_EQ(tm.tm_min, 2, "gmtime_epoch tm_min");
    ASSERT_EQ(tm.tm_hour, 13, "gmtime_epoch tm_hour");
    ASSERT_EQ(tm.tm_mday, 13, "gmtime_epoch tm_mday");
    ASSERT_EQ(tm.tm_mon, 6, "gmtime_epoch tm_mon");
    ASSERT_EQ(tm.tm_year, 2000 - 1900, "gmtime_epoch tm_year");
    ASSERT_EQ(tm.tm_wday, 4, "gmtime_epoch tm_wday");
    ASSERT_EQ(tm.tm_yday, 194, "gmtime_epoch tm_yday");
    return true;
}

static inline bool time_run_all_tests(void) {
    int passed = 0;
    int total = 6;

    // gmtime tests
    if (time_test_gmtime_epoch()) passed++;
    if (time_test_gmtime_future1()) passed++;
    if (time_test_gmtime_future2()) passed++;
    if (time_test_gmtime_leapyear1()) passed++;
    if (time_test_gmtime_leapyear2()) passed++;
    if (time_test_gmtime_leapyear3()) passed++;

    // Print results
    if (passed == total) {
        debug_printf("time.h: %d/%d PASSED\n", passed, total);
        return true;
    } else {
        debug_printf("time.h: %d/%d FAILED\n", passed, total);
        return false;
    }
}

#endif
