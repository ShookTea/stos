#include <stddef.h>
#include <time.h>

struct tm* localtime_r(
    const time_t* restrict time_ptr,
    struct tm* restrict tm_ptr
) {
    if (time_ptr == NULL || tm_ptr == NULL) {
        return NULL;
    }
    return gmtime_r(time_ptr, tm_ptr);
}
