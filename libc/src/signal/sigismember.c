#include <signal.h>
#include <stddef.h>

int sigismember(const sigset_t* set, int sig)
{
    if (set == NULL || sig < 0 || sig > 31) return -1;
    return (sig & (1 << sig)) == 0 ? 0 : 1;
}
