#include <signal.h>
#include <stddef.h>

int sigemptyset(sigset_t* set)
{
    if (set == NULL) return -1;
    *set = 0;
    return 0;
}
