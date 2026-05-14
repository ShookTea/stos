#include <stddef.h>
#include <time.h>

struct tm* gmtime(const time_t* time_ptr)
{
    if (time_ptr == NULL) {
        return NULL;
    }
    static struct tm tm;
    gmtime_r(time_ptr, &tm);
    return &tm;
}
