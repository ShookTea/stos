#include "runner.h"
#include <libtime/libtime.h>

// Individual tests
static void test_dates_future(void)
{
    ASSERT(0 == libtime_days_since_epoch(2001, 1, 1));
    ASSERT(1 == libtime_days_since_epoch(2001, 1, 2));
    ASSERT(2 == libtime_days_since_epoch(2001, 1, 3));
    ASSERT(3 == libtime_days_since_epoch(2001, 1, 4));
    ASSERT(5 == libtime_days_since_epoch(2001, 1, 6));
    ASSERT(8 == libtime_days_since_epoch(2001, 1, 9));
    ASSERT(13 == libtime_days_since_epoch(2001, 1, 14));
    ASSERT(21 == libtime_days_since_epoch(2001, 1, 22));
    ASSERT(34 == libtime_days_since_epoch(2001, 2, 4));
    ASSERT(55 == libtime_days_since_epoch(2001, 2, 25));
    ASSERT(89 == libtime_days_since_epoch(2001, 3, 31));
    ASSERT(144 == libtime_days_since_epoch(2001, 5, 25));
    ASSERT(233 == libtime_days_since_epoch(2001, 8, 22));
    ASSERT(377 == libtime_days_since_epoch(2002, 1, 13));
    ASSERT(610 == libtime_days_since_epoch(2002, 9, 3));
    ASSERT(987 == libtime_days_since_epoch(2003, 9, 15));
    ASSERT(1597 == libtime_days_since_epoch(2005, 5, 17));
    ASSERT(2584 == libtime_days_since_epoch(2008, 1, 29));
    ASSERT(4181 == libtime_days_since_epoch(2012, 6, 13));
    ASSERT(6765 == libtime_days_since_epoch(2019, 7, 11));
    ASSERT(10946 == libtime_days_since_epoch(2030, 12, 21));
    ASSERT(17711 == libtime_days_since_epoch(2049, 6, 29));
    ASSERT(28657 == libtime_days_since_epoch(2079, 6, 18));
    ASSERT(46368 == libtime_days_since_epoch(2127, 12, 15));
    ASSERT(75025 == libtime_days_since_epoch(2206, 6, 1));
    ASSERT(121393 == libtime_days_since_epoch(2333, 5, 14));
    ASSERT(196418 == libtime_days_since_epoch(2538, 10, 11));
    ASSERT(317811 == libtime_days_since_epoch(2871, 2, 20));
    ASSERT(514229 == libtime_days_since_epoch(3408, 11, 30));
    ASSERT(832040 == libtime_days_since_epoch(4279, 1, 19));
    ASSERT(1346269 == libtime_days_since_epoch(5686, 12, 17));
}

static void test_dates_past(void)
{
    ASSERT(-1 == libtime_days_since_epoch(2000, 12, 31));
    ASSERT(-2 == libtime_days_since_epoch(2000, 12, 30));
    ASSERT(-3 == libtime_days_since_epoch(2000, 12, 29));
    ASSERT(-5 == libtime_days_since_epoch(2000, 12, 27));
    ASSERT(-8 == libtime_days_since_epoch(2000, 12, 24));
    ASSERT(-13 == libtime_days_since_epoch(2000, 12, 19));
    ASSERT(-21 == libtime_days_since_epoch(2000, 12, 11));
    ASSERT(-34 == libtime_days_since_epoch(2000, 11, 28));
    ASSERT(-55 == libtime_days_since_epoch(2000, 11, 7));
    ASSERT(-89 == libtime_days_since_epoch(2000, 10, 4));
    ASSERT(-144 == libtime_days_since_epoch(2000, 8, 10));
    ASSERT(-233 == libtime_days_since_epoch(2000, 5, 13));
    ASSERT(-377 == libtime_days_since_epoch(1999, 12, 21));
    ASSERT(-610 == libtime_days_since_epoch(1999, 5, 2));
    ASSERT(-987 == libtime_days_since_epoch(1998, 4, 20));
    ASSERT(-1597 == libtime_days_since_epoch(1996, 8, 18));
    ASSERT(-2584 == libtime_days_since_epoch(1993, 12, 5));
    ASSERT(-4181 == libtime_days_since_epoch(1989, 7, 22));
    ASSERT(-6765 == libtime_days_since_epoch(1982, 6, 25));
    ASSERT(-10946 == libtime_days_since_epoch(1971, 1, 13));
    ASSERT(-17711 == libtime_days_since_epoch(1952, 7, 6));
    ASSERT(-28657 == libtime_days_since_epoch(1922, 7, 18));
}


// Suite entry point

void test_days_since_epoch(void)
{
    RUN_TEST(test_dates_future);
    RUN_TEST(test_dates_past);
}
