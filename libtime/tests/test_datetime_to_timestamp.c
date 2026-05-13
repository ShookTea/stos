#include "runner.h"
#include <libtime/libtime.h>

// Individual tests
static void test_zero_time(void)
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    ASSERT(0 == libtime_datetime_to_timestamp(&dt));
}

static void test_millis(void)
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.millis = 100;
    ASSERT(100 == libtime_datetime_to_timestamp(&dt));
    dt.millis = 400;
    ASSERT(400 == libtime_datetime_to_timestamp(&dt));
}

static void test_seconds(void)
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.second = 1;
    ASSERT(1000 == libtime_datetime_to_timestamp(&dt));
    dt.second = 2;
    ASSERT(2000 == libtime_datetime_to_timestamp(&dt));
    dt.millis = 100;
    ASSERT(2100 == libtime_datetime_to_timestamp(&dt));
}

static void test_minutes(void)
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.minute = 1;
    ASSERT(60000 == libtime_datetime_to_timestamp(&dt));
    dt.minute = 5;
    ASSERT(300000 == libtime_datetime_to_timestamp(&dt));
    dt.second = 3;
    ASSERT(303000 == libtime_datetime_to_timestamp(&dt));
    dt.millis = 650;
    ASSERT(303650 == libtime_datetime_to_timestamp(&dt));
}

static void test_hours(void)
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.hour = 1;
    ASSERT(3600000 == libtime_datetime_to_timestamp(&dt));
    dt.hour = 3;
    ASSERT(10800000 == libtime_datetime_to_timestamp(&dt));
    dt.minute = 1;
    ASSERT(10860000 == libtime_datetime_to_timestamp(&dt));
    dt.minute = 5;
    ASSERT(11100000 == libtime_datetime_to_timestamp(&dt));
    dt.second = 3;
    ASSERT(11103000 == libtime_datetime_to_timestamp(&dt));
    dt.millis = 650;
    ASSERT(11103650 == libtime_datetime_to_timestamp(&dt));
}

static void test_days(void)
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.day = 2;
    ASSERT(86400000LL == libtime_datetime_to_timestamp(&dt));
    dt.day = 8;
    ASSERT(7 * 86400000LL == libtime_datetime_to_timestamp(&dt));
    dt.day = 2;
    dt.hour = 1;
    dt.minute = 30;
    ASSERT(86400000LL + 5400000LL == libtime_datetime_to_timestamp(&dt));
}

static void test_months(void)
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.month = 2;
    ASSERT(31 * 86400000LL == libtime_datetime_to_timestamp(&dt));
    dt.month = 3;
    ASSERT(59 * 86400000LL == libtime_datetime_to_timestamp(&dt));
    libtime_datetime_t dt2 = { .year = 2004, .month = 3, .day = 1 };
    ASSERT(1155 * 86400000LL == libtime_datetime_to_timestamp(&dt2));
}

static void test_years(void)
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.year = 2002;
    ASSERT(365 * 86400000LL == libtime_datetime_to_timestamp(&dt));
    dt.year = 2005;
    ASSERT(1461 * 86400000LL == libtime_datetime_to_timestamp(&dt));
}

static void test_before_epoch(void)
{
    libtime_datetime_t dt = { .year = 2000, .month = 12, .day = 31 };
    ASSERT(-86400000LL == libtime_datetime_to_timestamp(&dt));
    dt.hour = 23;
    dt.minute = 59;
    dt.second = 59;
    dt.millis = 999;
    ASSERT(-1LL == libtime_datetime_to_timestamp(&dt));
    libtime_datetime_t dt2 = { .year = 2000, .month = 1, .day = 1 };
    ASSERT(-366 * 86400000LL == libtime_datetime_to_timestamp(&dt2));
}

void test_datetime_to_timestamp(void)
{
    RUN_TEST(test_zero_time);
    RUN_TEST(test_millis);
    RUN_TEST(test_seconds);
    RUN_TEST(test_minutes);
    RUN_TEST(test_hours);
    RUN_TEST(test_days);
    RUN_TEST(test_months);
    RUN_TEST(test_years);
    RUN_TEST(test_before_epoch);
}
