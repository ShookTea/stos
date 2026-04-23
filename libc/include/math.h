#ifndef _MATH_H
#define _MATH_H 1

#include <stdint.h>
#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns a non-zero value if and only if its argument has a NaN value.
 */
int isnan(double x);

/**
 * Returns a non-zero value if and only if its argument has an infinite value.
 */
int isinf(double x);

/**
 * Rounds up given value towards positive infinity.
 */
double ceil(double x);

#ifdef __cplusplus
}
#endif

#endif
