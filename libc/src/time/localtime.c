#include <stddef.h>
#include <time.h>

struct tm* localtime(const time_t* time_ptr)
{
    if (time_ptr == NULL) {
        return NULL;
    }
    return gmtime(time_ptr);
}
