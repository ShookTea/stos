#ifndef KERNEL_LIBC_TEST_BASE_H
#define KERNEL_LIBC_TEST_BASE_H

#include <stdbool.h>
#include <string.h>

/**
 * Base Test Utilities for libc Test Suite
 *
 * Common macros and utilities used across all libc tests
 */

/**
 * Assert that two integer values are equal
 */
#define ASSERT_EQ(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            debug_printf("  FAILED: %s (expected %d, got %d)\n", msg, (int)(expected), (int)(actual)); \
            return false; \
        } \
    } while(0)

/**
 * Assert that two strings are equal
 */
#define ASSERT_STR_EQ(actual, expected, msg) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            debug_printf("  FAILED: %s (expected \"%s\", got \"%s\")\n", msg, expected, actual); \
            return false; \
        } \
    } while(0)

/**
 * Assert that a condition is true
 */
#define ASSERT_TRUE(condition, msg) \
    do { \
        if (!(condition)) { \
            debug_printf("  FAILED: %s\n", msg); \
            return false; \
        } \
    } while(0)

/**
 * Assert that a condition is false
 */
#define ASSERT_FALSE(condition, msg) \
    do { \
        if (condition) { \
            debug_printf("  FAILED: %s\n", msg); \
            return false; \
        } \
    } while(0)

/**
 * Assert that two pointers are equal
 */
#define ASSERT_PTR_EQ(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            debug_printf("  FAILED: %s (expected %p, got %p)\n", msg, (void*)(expected), (void*)(actual)); \
            return false; \
        } \
    } while(0)

/**
 * Assert that a pointer is NULL
 */
#define ASSERT_NULL(ptr, msg) \
    do { \
        if ((ptr) != NULL) { \
            debug_printf("  FAILED: %s (expected NULL, got %p)\n", msg, (void*)(ptr)); \
            return false; \
        } \
    } while(0)

/**
 * Assert that a pointer is not NULL
 */
#define ASSERT_NOT_NULL(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            debug_printf("  FAILED: %s (expected non-NULL, got NULL)\n", msg); \
            return false; \
        } \
    } while(0)

#endif // KERNEL_LIBC_TEST_BASE_H
