#include "runner.h"
#include <libtime/libtime.h>

static void assert_datetime(libtime_datetime_t* dt,
    uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t second, uint16_t millis)
{
    ASSERT(dt->year == year);
    ASSERT(dt->month == month);
    ASSERT(dt->day == day);
    ASSERT(dt->hour == hour);
    ASSERT(dt->minute == minute);
    ASSERT(dt->second == second);
    ASSERT(dt->millis == millis);
}

static void test_epoch_zero()
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(libtime_unix_to_timestamp(978307200LL), &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 0, 0, 0);
}

static void test_unix_zero()
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(libtime_unix_to_timestamp(0), &dt);
    assert_datetime(&dt, 1970, 1, 1, 0, 0, 0, 0);
}

static void test_future()
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(libtime_unix_to_timestamp(1767225600LL), &dt);
    assert_datetime(&dt, 2026, 1, 1, 0, 0, 0, 0);
}

static void test_between()
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(libtime_unix_to_timestamp(410227200LL), &dt);
    assert_datetime(&dt, 1983, 1, 1, 0, 0, 0, 0);
}

static void test_past()
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(libtime_unix_to_timestamp(-536457600LL), &dt);
    assert_datetime(&dt, 1953, 1, 1, 0, 0, 0, 0);
}

void test_unix_to_timestamp(void)
{
    RUN_TEST(test_epoch_zero);
    RUN_TEST(test_unix_zero);
    RUN_TEST(test_future);
    RUN_TEST(test_between);
    RUN_TEST(test_past);
}
