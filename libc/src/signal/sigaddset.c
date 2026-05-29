#include <signal.h>
#include <stddef.h>
#include <stdint.h>

int sigaddset(sigset_t* set, int sig)
{
    if (set == NULL || sig < 0 || sig > 31) return -1;
    *set |= (1 << sig);
    return 0;
}
