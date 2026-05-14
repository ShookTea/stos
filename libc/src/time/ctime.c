#include <stddef.h>
#include <time.h>

char* ctime(const time_t* time_ptr)
{
    return asctime(localtime(time_ptr));
}
