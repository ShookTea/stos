#include <math.h>
#include <stdint.h>
#include <string.h>

int isnan(double x)
{
    uint64_t u;
    memcpy(&u, &x, sizeof u);
    return ((u & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) &&
           ((u & 0x000FFFFFFFFFFFFFULL) != 0);
}
