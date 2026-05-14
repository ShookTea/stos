#include <stddef.h>
#include <time.h>

char* ctime_r(const time_t* restrict time_ptr, char buf[restrict 26])
{
    return asctime_r(localtime(time_ptr), buf);
}
