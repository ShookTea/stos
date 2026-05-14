#include "runner.h"
#include <libtime/libtime.h>

// Individual tests
static void test_zero_time(void)
{
    ASSERT(1 == libtime_weekday_from_days(2001, 1, 1));
}

static void test_following_days(void)
{
    ASSERT(2 == libtime_weekday_from_days(2001, 1, 2));
    ASSERT(3 == libtime_weekday_from_days(2001, 1, 3));
    ASSERT(4 == libtime_weekday_from_days(2001, 1, 4));
    ASSERT(5 == libtime_weekday_from_days(2001, 1, 5));
    ASSERT(6 == libtime_weekday_from_days(2001, 1, 6));
    ASSERT(0 == libtime_weekday_from_days(2001, 1, 7));
    ASSERT(1 == libtime_weekday_from_days(2001, 1, 8));
    ASSERT(2 == libtime_weekday_from_days(2001, 1, 9));
    ASSERT(3 == libtime_weekday_from_days(2001, 1, 10));
    ASSERT(4 == libtime_weekday_from_days(2001, 1, 11));
    ASSERT(5 == libtime_weekday_from_days(2001, 1, 12));
    ASSERT(6 == libtime_weekday_from_days(2001, 1, 13));
}

static void test_far_future(void)
{
    ASSERT(4 == libtime_weekday_from_days(2026, 5, 14));
    ASSERT(2 == libtime_weekday_from_days(2137, 4, 2));
}

static void test_past(void)
{
    ASSERT(0 == libtime_weekday_from_days(2000, 12, 31));
    ASSERT(6 == libtime_weekday_from_days(2000, 12, 30));
    ASSERT(5 == libtime_weekday_from_days(2000, 12, 29));
    ASSERT(4 == libtime_weekday_from_days(2000, 12, 28));
    ASSERT(3 == libtime_weekday_from_days(2000, 12, 27));
    ASSERT(2 == libtime_weekday_from_days(2000, 12, 26));
    ASSERT(1 == libtime_weekday_from_days(2000, 12, 25));
    ASSERT(0 == libtime_weekday_from_days(2000, 12, 24));
    ASSERT(6 == libtime_weekday_from_days(2000, 12, 23));
    ASSERT(4 == libtime_weekday_from_days(2000, 2, 17));
    ASSERT(2 == libtime_weekday_from_days(1997, 10, 21));
}

void test_weekday_from_days(void)
{
    RUN_TEST(test_zero_time);
    RUN_TEST(test_following_days);
    RUN_TEST(test_far_future);
    RUN_TEST(test_past);
}
