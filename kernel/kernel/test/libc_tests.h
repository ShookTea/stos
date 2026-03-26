#ifndef KERNEL_LIBC_TESTS_H
#define KERNEL_LIBC_TESTS_H

#include "libc/string_tests.h"
#include "libc/stdlib_tests.h"
#include "libc/ctype_tests.h"
#include "stdlib.h"

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
    bool success = true;

    if (!string_run_all_tests()) {
        success = false;
    }
    if (!stdlib_run_all_tests()) {
        success = false;
    }
    if (!ctype_run_all_tests()) {
        success = false;
    }

    printf("=====================================\n");

    if (!success) {
        abort();
    }
}

#endif
