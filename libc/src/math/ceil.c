#include <math.h>
#include <string.h>

double ceil(double x)
{
    uint64_t u;
    memcpy(&u, &x, sizeof u);

    int e = (int)((u >> 52) & 0x7FF) - 0x3FF;
    uint64_t sign = u >> 63;
    uint64_t mask;

    if (e >= 52) {
        // already integral, inf, or nan
        return x;
    }

    if (e < 0) {
        if (x == 0.0) {
            return x; // Preserve signed zero
        }
        // negatives go to -0, positives to 1
        return sign ? -0.0 : 1.0;
    }

    mask = (1ULL << (52 - e)) - 1;
    if ((u & mask) == 0) {
        // Already integral
        return x;
    }

    if (!sign) {
        // Positive non-integer: round upward
        u += 1ULL << (52 - e);
    }

    // clear fractional bits
    u &= ~mask;

    memcpy(&x, &u, sizeof x);
    return x;
}
