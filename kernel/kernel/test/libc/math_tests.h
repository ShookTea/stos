#ifndef KERNEL_LIBC_MATH_TESTS_H
#define KERNEL_LIBC_MATH_TESTS_H

#include <math.h>
#include "kernel/debug.h"
#include "test_base.h"

static inline bool math_test_isinf_infinite(void) {
    double pos_inf = 1.0 / 0.0;
    double neg_inf = -1.0 / 0.0;
    ASSERT_TRUE(isinf(pos_inf) != 0, "positive infinity is infinite");
    ASSERT_TRUE(isinf(neg_inf) != 0, "negative infinity is infinite");
    ASSERT_EQ(isinf(pos_inf), 1, "positive infinity returns 1");
    ASSERT_EQ(isinf(neg_inf), -1, "negative infinity returns -1");
    return true;
}

static inline bool math_test_isinf_finite(void) {
    ASSERT_EQ(isinf(0.0), 0, "zero is not infinite");
    ASSERT_EQ(isinf(1.0), 0, "one is not infinite");
    ASSERT_EQ(isinf(-1.0), 0, "negative one is not infinite");
    ASSERT_EQ(isinf(3.14), 0, "pi approx is not infinite");
    return true;
}

static inline bool math_test_isinf_nan(void) {
    double nan_val = 0.0 / 0.0;
    ASSERT_EQ(isinf(nan_val), 0, "NaN is not infinite");
    return true;
}

static inline bool math_test_isnan_nan(void) {
    double nan_val = 0.0 / 0.0;
    ASSERT_TRUE(isnan(nan_val) != 0, "NaN is NaN");
    return true;
}

static inline bool math_test_isnan_finite(void) {
    ASSERT_EQ(isnan(0.0), 0, "zero is not NaN");
    ASSERT_EQ(isnan(1.0), 0, "one is not NaN");
    ASSERT_EQ(isnan(-1.0), 0, "negative one is not NaN");
    ASSERT_EQ(isnan(3.14), 0, "pi approx is not NaN");
    return true;
}

static inline bool math_test_isnan_infinite(void) {
    double pos_inf = 1.0 / 0.0;
    double neg_inf = -1.0 / 0.0;
    ASSERT_EQ(isnan(pos_inf), 0, "positive infinity is not NaN");
    ASSERT_EQ(isnan(neg_inf), 0, "negative infinity is not NaN");
    return true;
}

static inline bool math_test_ceil_integer(void) {
    ASSERT_EQ((int)ceil(1.0), 1, "ceil(1.0) == 1");
    ASSERT_EQ((int)ceil(0.0), 0, "ceil(0.0) == 0");
    ASSERT_EQ((int)ceil(-1.0), -1, "ceil(-1.0) == -1");
    return true;
}

static inline bool math_test_ceil_positive(void) {
    ASSERT_EQ((int)ceil(1.1), 2, "ceil(1.1) == 2");
    ASSERT_EQ((int)ceil(1.5), 2, "ceil(1.5) == 2");
    ASSERT_EQ((int)ceil(1.9), 2, "ceil(1.9) == 2");
    ASSERT_EQ((int)ceil(0.1), 1, "ceil(0.1) == 1");
    return true;
}

static inline bool math_test_ceil_negative(void) {
    ASSERT_EQ((int)ceil(-1.1), -1, "ceil(-1.1) == -1");
    ASSERT_EQ((int)ceil(-1.5), -1, "ceil(-1.5) == -1");
    ASSERT_EQ((int)ceil(-1.9), -1, "ceil(-1.9) == -1");
    ASSERT_EQ((int)ceil(-0.1), 0, "ceil(-0.1) == 0");
    return true;
}

static inline bool math_run_all_tests(void) {
    debug_printf("\n=== Math Library Tests ===\n");
    int passed = 0;
    int total = 9;

    if (math_test_isinf_infinite()) passed++;
    if (math_test_isinf_finite()) passed++;
    if (math_test_isinf_nan()) passed++;
    if (math_test_isnan_nan()) passed++;
    if (math_test_isnan_finite()) passed++;
    if (math_test_isnan_infinite()) passed++;
    if (math_test_ceil_integer()) passed++;
    if (math_test_ceil_positive()) passed++;
    if (math_test_ceil_negative()) passed++;

    if (passed == total) {
        debug_printf("Math: %d/%d PASSED\n", passed, total);
        return true;
    } else {
        debug_printf("Math: %d/%d FAILED\n", passed, total);
        return false;
    }
}

#endif // KERNEL_LIBC_MATH_TESTS_H
