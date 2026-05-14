#include <time.h>
#include <stdint.h>
#include <libtime/libtime.h>

time_t libtime_timestamp_to_unix(int64_t timestamp)
{
    return (timestamp / 1000) + 978307200LL;
}
