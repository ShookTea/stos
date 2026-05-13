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

void test_datetime_to_timestamp(void)
{
    RUN_TEST(test_zero_time);
    RUN_TEST(test_millis);
    RUN_TEST(test_seconds);
    RUN_TEST(test_minutes);
    RUN_TEST(test_hours);
}
