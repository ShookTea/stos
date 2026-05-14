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
    ASSERT_EQ(0, tm.tm_sec, "gmtime_epoch tm_sec");
    ASSERT_EQ(0, tm.tm_min, "gmtime_epoch tm_min");
    ASSERT_EQ(0, tm.tm_hour, "gmtime_epoch tm_hour");
    ASSERT_EQ(1, tm.tm_mday, "gmtime_epoch tm_mday");
    ASSERT_EQ(0, tm.tm_mon, "gmtime_epoch tm_mon");
    ASSERT_EQ(1970 - 1900, tm.tm_year, "gmtime_epoch tm_year");
    ASSERT_EQ(4, tm.tm_wday, "gmtime_epoch tm_wday");
    ASSERT_EQ(0, tm.tm_yday, "gmtime_epoch tm_yday");
    return true;
}

static inline bool time_test_gmtime_future1(void) {
    struct tm tm;
    time_t ts = 1778748085LL; // 2026-05-14
    gmtime_r(&ts, &tm);
    ASSERT_EQ(25, tm.tm_sec, "gmtime_future1 tm_sec");
    ASSERT_EQ(41, tm.tm_min, "gmtime_future1 tm_min");
    ASSERT_EQ(8, tm.tm_hour, "gmtime_future1 tm_hour");
    ASSERT_EQ(14, tm.tm_mday, "gmtime_future1 tm_mday");
    ASSERT_EQ(4, tm.tm_mon, "gmtime_future1 tm_mon");
    ASSERT_EQ(2026 - 1900, tm.tm_year, "gmtime_future1 tm_year");
    ASSERT_EQ(4, tm.tm_wday, "gmtime_future1 tm_wday");
    ASSERT_EQ(133, tm.tm_yday, "gmtime_future1 tm_yday");
    return true;
}

static inline bool time_test_gmtime_future2(void) {
    struct tm tm;
    time_t ts = 5299480968LL; // 2137-12-07
    gmtime_r(&ts, &tm);
    ASSERT_EQ(48, tm.tm_sec, "gmtime_future2 tm_sec");
    ASSERT_EQ(2, tm.tm_min, "gmtime_future2 tm_min");
    ASSERT_EQ(14, tm.tm_hour, "gmtime_future2 tm_hour");
    ASSERT_EQ(7, tm.tm_mday, "gmtime_future2 tm_mday");
    ASSERT_EQ(11, tm.tm_mon, "gmtime_future2 tm_mon");
    ASSERT_EQ(2137 - 1900, tm.tm_year, "gmtime_future2 tm_year");
    ASSERT_EQ(6, tm.tm_wday, "gmtime_future2 tm_wday");
    ASSERT_EQ(340, tm.tm_yday, "gmtime_future2 tm_yday");
    return true;
}

static inline bool time_test_gmtime_leapyear1(void) {
    struct tm tm;
    time_t ts = 1468414968LL; // 2016-07-13 (handle leap year)
    gmtime_r(&ts, &tm);
    ASSERT_EQ(48, tm.tm_sec, "gmtime_leapyear1 tm_sec");
    ASSERT_EQ(2, tm.tm_min, "gmtime_leapyear1 tm_min");
    ASSERT_EQ(13, tm.tm_hour, "gmtime_leapyear1 tm_hour");
    ASSERT_EQ(13, tm.tm_mday, "gmtime_leapyear1 tm_mday");
    ASSERT_EQ(6, tm.tm_mon, "gmtime_leapyear1 tm_mon");
    ASSERT_EQ(2016 - 1900, tm.tm_year, "gmtime_leapyear1 tm_year");
    ASSERT_EQ(3, tm.tm_wday, "gmtime_leapyear1 tm_wday");
    ASSERT_EQ(194, tm.tm_yday, "gmtime_leapyear1 tm_yday");
    return true;
}

static inline bool time_test_gmtime_leapyear2(void) {
    struct tm tm;
    time_t ts = 4119166968LL; // 2100-07-13 (not a leap year)
    gmtime_r(&ts, &tm);
    ASSERT_EQ(48, tm.tm_sec, "gmtime_leapyear2 tm_sec");
    ASSERT_EQ(2, tm.tm_min, "gmtime_leapyear2 tm_min");
    ASSERT_EQ(13, tm.tm_hour, "gmtime_leapyear2 tm_hour");
    ASSERT_EQ(13, tm.tm_mday, "gmtime_leapyear2 tm_mday");
    ASSERT_EQ(6, tm.tm_mon, "gmtime_leapyear2 tm_mon");
    ASSERT_EQ(2100 - 1900, tm.tm_year, "gmtime_leapyear2 tm_year");
    ASSERT_EQ(2, tm.tm_wday, "gmtime_leapyear2 tm_wday");
    ASSERT_EQ(193, tm.tm_yday, "gmtime_leapyear2 tm_yday");
    return true;
}

static inline bool time_test_gmtime_leapyear3(void) {
    struct tm tm;
    time_t ts = 963493368LL; // 2000-07-13 (a leap year)
    gmtime_r(&ts, &tm);
    ASSERT_EQ(48, tm.tm_sec, "gmtime_leapyear3 tm_sec");
    ASSERT_EQ(2, tm.tm_min, "gmtime_leapyear3 tm_min");
    ASSERT_EQ(13, tm.tm_hour, "gmtime_leapyear3 tm_hour");
    ASSERT_EQ(13, tm.tm_mday, "gmtime_leapyear3 tm_mday");
    ASSERT_EQ(6, tm.tm_mon, "gmtime_leapyear3 tm_mon");
    ASSERT_EQ(2000 - 1900, tm.tm_year, "gmtime_leapyear3 tm_year");
    ASSERT_EQ(4, tm.tm_wday, "gmtime_leapyear3 tm_wday");
    ASSERT_EQ(194, tm.tm_yday, "gmtime_leapyear3 tm_yday");
    return true;
}

static inline bool time_test_gmtime_past1(void) {
    struct tm tm;
    time_t ts = -506426232LL; // 1953-12-14
    gmtime_r(&ts, &tm);
    ASSERT_EQ(48, tm.tm_sec, "gmtime_past1 tm_sec");
    ASSERT_EQ(2, tm.tm_min, "gmtime_past1 tm_min");
    ASSERT_EQ(14, tm.tm_hour, "gmtime_past1 tm_hour");
    ASSERT_EQ(14, tm.tm_mday, "gmtime_past1 tm_mday");
    ASSERT_EQ(11, tm.tm_mon, "gmtime_past1 tm_mon");
    ASSERT_EQ(1953 - 1900, tm.tm_year, "gmtime_past1 tm_year");
    ASSERT_EQ(1, tm.tm_wday, "gmtime_past1 tm_wday");
    ASSERT_EQ(347, tm.tm_yday, "gmtime_past1 tm_yday");
    return true;
}

#define TM_ZERO { .tm_sec = 0, .tm_min = 0, .tm_hour = 0, .tm_mday = 0, .tm_mon = 0, .tm_year = 0, .tm_wday = 0, .tm_yday = 0 }

// Normalize 65 seconds into 1 minute 5 seconds
static inline bool time_test_mktime_normalize1(void) {
    struct tm tm = TM_ZERO;
    tm.tm_year = 2016 - 1900;
    tm.tm_mday = 1;
    tm.tm_sec = 65;
    mktime(&tm);
    ASSERT_EQ(5, tm.tm_sec, "mktime_normalize1 tm_sec");
    ASSERT_EQ(1, tm.tm_min, "mktime_normalize1 tm_min");
    ASSERT_EQ(0, tm.tm_hour, "mktime_normalize1 tm_hour");
    ASSERT_EQ(1, tm.tm_mday, "mktime_normalize1 tm_mday");
    ASSERT_EQ(0, tm.tm_mon, "mktime_normalize1 tm_mon");
    ASSERT_EQ(2016 - 1900, tm.tm_year, "mktime_normalize1 tm_year");
    ASSERT_EQ(5, tm.tm_wday, "mktime_normalize1 tm_wday");
    ASSERT_EQ(0, tm.tm_yday, "mktime_normalize1 tm_yday");
    return true;
}

// Normalize 79 hours into 3 days 7 hours
static inline bool time_test_mktime_normalize2(void) {
    struct tm tm = TM_ZERO;
    tm.tm_year = 2016 - 1900;
    tm.tm_mday = 1;
    tm.tm_hour = 79;
    mktime(&tm);
    ASSERT_EQ(0, tm.tm_sec, "mktime_normalize2 tm_sec");
    ASSERT_EQ(0, tm.tm_min, "mktime_normalize2 tm_min");
    ASSERT_EQ(7, tm.tm_hour, "mktime_normalize2 tm_hour");
    ASSERT_EQ(4, tm.tm_mday, "mktime_normalize2 tm_mday");
    ASSERT_EQ(0, tm.tm_mon, "mktime_normalize2 tm_mon");
    ASSERT_EQ(2016 - 1900, tm.tm_year, "mktime_normalize2 tm_year");
    ASSERT_EQ(1, tm.tm_wday, "mktime_normalize2 tm_wday");
    ASSERT_EQ(3, tm.tm_yday, "mktime_normalize2 tm_yday");
    return true;
}

// Normalize Jan 33rd into Feb 2nd
static inline bool time_test_mktime_normalize3(void) {
    struct tm tm = TM_ZERO;
    tm.tm_year = 2016 - 1900;
    tm.tm_mday = 33;
    mktime(&tm);
    ASSERT_EQ(0, tm.tm_sec, "mktime_normalize3 tm_sec");
    ASSERT_EQ(0, tm.tm_min, "mktime_normalize3 tm_min");
    ASSERT_EQ(0, tm.tm_hour, "mktime_normalize3 tm_hour");
    ASSERT_EQ(2, tm.tm_mday, "mktime_normalize3 tm_mday");
    ASSERT_EQ(1, tm.tm_mon, "mktime_normalize3 tm_mon");
    ASSERT_EQ(2016 - 1900, tm.tm_year, "mktime_normalize3 tm_year");
    ASSERT_EQ(2, tm.tm_wday, "mktime_normalize3 tm_wday");
    ASSERT_EQ(32, tm.tm_yday, "mktime_normalize3 tm_yday");
    return true;
}

// Normalize Jan 512nd 1970 into May 27th 1971
static inline bool time_test_mktime_normalize4(void) {
    struct tm tm = TM_ZERO;
    tm.tm_year = 1970 - 1900;
    tm.tm_mday = 512;
    mktime(&tm);
    ASSERT_EQ(0, tm.tm_sec, "mktime_normalize4 tm_sec");
    ASSERT_EQ(0, tm.tm_min, "mktime_normalize4 tm_min");
    ASSERT_EQ(0, tm.tm_hour, "mktime_normalize4 tm_hour");
    ASSERT_EQ(27, tm.tm_mday, "mktime_normalize4 tm_mday");
    ASSERT_EQ(4, tm.tm_mon, "mktime_normalize4 tm_mon");
    ASSERT_EQ(1971 - 1900, tm.tm_year, "mktime_normalize4 tm_year");
    ASSERT_EQ(4, tm.tm_wday, "mktime_normalize4 tm_wday");
    ASSERT_EQ(146, tm.tm_yday, "mktime_normalize4 tm_yday");
    return true;
}

// Normalize Jan -3rd 1970 into Dec 29th 1969
static inline bool time_test_mktime_normalize5(void) {
    struct tm tm = TM_ZERO;
    tm.tm_year = 1970 - 1900;
    tm.tm_mday = -2; // days are 1-indexed
    mktime(&tm);
    ASSERT_EQ(0, tm.tm_sec, "mktime_normalize5 tm_sec");
    ASSERT_EQ(0, tm.tm_min, "mktime_normalize5 tm_min");
    ASSERT_EQ(0, tm.tm_hour, "mktime_normalize5 tm_hour");
    ASSERT_EQ(29, tm.tm_mday, "mktime_normalize5 tm_mday");
    ASSERT_EQ(11, tm.tm_mon, "mktime_normalize5 tm_mon");
    ASSERT_EQ(1969 - 1900, tm.tm_year, "mktime_normalize5 tm_year");
    ASSERT_EQ(1, tm.tm_wday, "mktime_normalize5 tm_wday");
    ASSERT_EQ(362, tm.tm_yday, "mktime_normalize5 tm_yday");
    return true;
}

// Calculate timestamp for 2026-11-14 14:02:48 UTC
static inline bool time_test_mktime_calc1(void) {
    struct tm tm = TM_ZERO;
    tm.tm_year = 2026 - 1900;
    tm.tm_mon = 11 - 1;
    tm.tm_mday = 14;
    tm.tm_hour = 14;
    tm.tm_min = 2;
    tm.tm_sec = 48;
    time_t res = mktime(&tm);
    ASSERT_EQ(res, 1794664968LL, "mktime_calc1 result");
    return true;
}

static inline bool time_run_all_tests(void) {
    int passed = 0;
    int total = 13;

    // gmtime tests
    if (time_test_gmtime_epoch()) passed++;
    if (time_test_gmtime_future1()) passed++;
    if (time_test_gmtime_future2()) passed++;
    if (time_test_gmtime_leapyear1()) passed++;
    if (time_test_gmtime_leapyear2()) passed++;
    if (time_test_gmtime_leapyear3()) passed++;
    if (time_test_gmtime_past1()) passed++;

    // mktime tests
    if (time_test_mktime_normalize1()) passed++;
    if (time_test_mktime_normalize2()) passed++;
    if (time_test_mktime_normalize3()) passed++;
    if (time_test_mktime_normalize4()) passed++;
    if (time_test_mktime_normalize5()) passed++;
    if (time_test_mktime_calc1()) passed++;

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
