#include "runner.h"
#include <libtime/libtime.h>

static void test_epoch_zero()
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    ASSERT(978307200LL == libtime_timestamp_to_unix(libtime_datetime_to_timestamp(&dt)));
}

static void test_unix_zero()
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.year = 1970;
    ASSERT(0 == libtime_timestamp_to_unix(libtime_datetime_to_timestamp(&dt)));
}

static void test_future()
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.year = 2026;
    ASSERT(1767225600LL == libtime_timestamp_to_unix(libtime_datetime_to_timestamp(&dt)));
}

static void test_between()
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.year = 1983;
    ASSERT(410227200LL == libtime_timestamp_to_unix(libtime_datetime_to_timestamp(&dt)));
}

static void test_past()
{
    libtime_datetime_t dt = LIBTIME_DATETIME_EPOCH;
    dt.year = 1953;
    ASSERT(-536457600LL == libtime_timestamp_to_unix(libtime_datetime_to_timestamp(&dt)));
}

void test_timestamp_to_unix(void)
{
    RUN_TEST(test_epoch_zero);
    RUN_TEST(test_unix_zero);
    RUN_TEST(test_future);
    RUN_TEST(test_between);
    RUN_TEST(test_past);
}
