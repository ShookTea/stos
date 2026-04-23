#include <math.h>
#include <stdint.h>
#include <string.h>

int isinf(double x)
{
    uint64_t u;
    memcpy(&u, &x, sizeof u);

    if ((u & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF0000000000000ULL) {
        return (u >> 63) ? -1 : 1;
    }
    return 0;
}
