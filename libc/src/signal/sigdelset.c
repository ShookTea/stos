#include <signal.h>
#include <stddef.h>

int sigdelset(sigset_t* set, int sig)
{
    if (set == NULL || sig < 0 || sig > 31) return -1;
    *set &= ~(1 << sig);
    return 0;
}
