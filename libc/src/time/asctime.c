#include <stddef.h>
#include <time.h>

char* asctime(const struct tm* tm_ptr)
{
    if (tm_ptr == NULL) {
        return NULL;
    }
    static char buf[26];
    asctime_r(tm_ptr, buf);
    return buf;
}
