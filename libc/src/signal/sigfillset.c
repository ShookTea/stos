#include <signal.h>
#include <stddef.h>
#include <stdint.h>

int sigfillset(sigset_t* set)
{
    if (set == NULL) return -1;
    *set = UINT32_MAX;
    return 0;
}
