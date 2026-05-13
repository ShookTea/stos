#include "runner.h"
#include <libtime/libtime.h>
#include <stdio.h>

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

static void test_zero_time(void)
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(0, &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 0, 0, 0);
}

static void test_millis(void)
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(100, &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 0, 0, 100);
    libtime_timestamp_to_datetime(400, &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 0, 0, 400);
}

static void test_seconds(void)
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(1000, &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 0, 1, 0);
    libtime_timestamp_to_datetime(2000, &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 0, 2, 0);
    libtime_timestamp_to_datetime(2100, &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 0, 2, 100);
}

static void test_minutes(void)
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(60000, &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 1, 0, 0);
    libtime_timestamp_to_datetime(300000, &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 5, 0, 0);
    libtime_timestamp_to_datetime(303000, &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 5, 3, 0);
    libtime_timestamp_to_datetime(303650, &dt);
    assert_datetime(&dt, 2001, 1, 1, 0, 5, 3, 650);
}

static void test_hours(void)
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(3600000, &dt);
    assert_datetime(&dt, 2001, 1, 1, 1, 0, 0, 0);
    libtime_timestamp_to_datetime(10800000, &dt);
    assert_datetime(&dt, 2001, 1, 1, 3, 0, 0, 0);
    libtime_timestamp_to_datetime(10860000, &dt);
    assert_datetime(&dt, 2001, 1, 1, 3, 1, 0, 0);
    libtime_timestamp_to_datetime(11100000, &dt);
    assert_datetime(&dt, 2001, 1, 1, 3, 5, 0, 0);
    libtime_timestamp_to_datetime(11103000, &dt);
    assert_datetime(&dt, 2001, 1, 1, 3, 5, 3, 0);
    libtime_timestamp_to_datetime(11103650, &dt);
    assert_datetime(&dt, 2001, 1, 1, 3, 5, 3, 650);
}

static void test_days(void)
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(86400000LL, &dt);
    assert_datetime(&dt, 2001, 1, 2, 0, 0, 0, 0);
    libtime_timestamp_to_datetime(7 * 86400000LL, &dt);
    assert_datetime(&dt, 2001, 1, 8, 0, 0, 0, 0);
    libtime_timestamp_to_datetime(86400000LL + 5400000LL, &dt);
    assert_datetime(&dt, 2001, 1, 2, 1, 30, 0, 0);
}

static void test_months(void)
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(31 * 86400000LL, &dt);
    assert_datetime(&dt, 2001, 2, 1, 0, 0, 0, 0);
    libtime_timestamp_to_datetime(59 * 86400000LL, &dt);
    assert_datetime(&dt, 2001, 3, 1, 0, 0, 0, 0);
    libtime_timestamp_to_datetime(1155 * 86400000LL, &dt);
    assert_datetime(&dt, 2004, 3, 1, 0, 0, 0, 0);
}

static void test_years(void)
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(365 * 86400000LL, &dt);
    assert_datetime(&dt, 2002, 1, 1, 0, 0, 0, 0);
    libtime_timestamp_to_datetime(1461 * 86400000LL, &dt);
    assert_datetime(&dt, 2005, 1, 1, 0, 0, 0, 0);
}

static void test_before_epoch(void)
{
    libtime_datetime_t dt;
    libtime_timestamp_to_datetime(-86400000LL, &dt);
    assert_datetime(&dt, 2000, 12, 31, 0, 0, 0, 0);
    libtime_timestamp_to_datetime(-1LL, &dt);
    assert_datetime(&dt, 2000, 12, 31, 23, 59, 59, 999);
    libtime_timestamp_to_datetime(-366 * 86400000LL, &dt);
    assert_datetime(&dt, 2000, 1, 1, 0, 0, 0, 0);
}

void test_timestamp_to_datetime(void)
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
