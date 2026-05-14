#include <time.h>
#include <stdint.h>
#include <libtime/libtime.h>

int64_t libtime_unix_to_timestamp(time_t unix_ts)
{
    return (((int64_t)unix_ts) - 978307200LL) * 1000;
}
