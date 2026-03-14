#ifndef KERNEL_LIBC_TESTS_H
#define KERNEL_LIBC_TESTS_H

#include "libc/string_tests.h"
#include "libc/stdlib_tests.h"
#include "libc/ctype_tests.h"

/**
 * Main libc Test Suite Runner
 *
 * Runs all libc tests (string, stdlib, ctype)
 */

/**
 * Run all libc tests
 */
static inline void libc_run_all_tests(void) {
    printf("\n========== LIBC TEST SUITE ==========\n");

    // Run string tests
    string_run_all_tests();

    // Run stdlib tests
    stdlib_run_all_tests();

    // Run ctype tests
    ctype_run_all_tests();

    printf("=====================================\n");
}

#endif // KERNEL_LIBC_TESTS_H
