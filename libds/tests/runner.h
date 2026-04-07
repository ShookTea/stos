#ifndef LIBDS_RUNNER_H
#define LIBDS_RUNNER_H

#include <stdio.h>

extern int _failures;
extern int _total;

#define ASSERT(cond) do { \
    _total++; \
    if (!(cond)) { \
        printf("    FAIL: %s (line %d)\n", #cond, __LINE__); \
        _failures++; \
    } \
} while(0)

#define RUN_TEST(fn) do { \
    printf("  %-44s", #fn); \
    int _f = _failures; \
    fn(); \
    printf("%s\n", (_failures == _f) ? "ok" : "FAILED"); \
} while(0)

#endif
