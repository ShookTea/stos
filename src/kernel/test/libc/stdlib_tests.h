#ifndef KERNEL_LIBC_STDLIB_TESTS_H
#define KERNEL_LIBC_STDLIB_TESTS_H

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include "test_base.h"

/**
 * Stdlib Library Test Suite
 *
 * Comprehensive tests for stdlib.h functions (atoi, strtol, strtoul)
 * Note: abort() is not tested as it causes kernel panic
 */

/**
 * Test atoi: basic positive numbers
 */
static inline bool stdlib_test_atoi_positive(void) {
    printf("\n[STDLIB TEST 1] atoi - Positive Numbers\n");

    ASSERT_EQ(atoi("0"), 0, "zero");
    ASSERT_EQ(atoi("1"), 1, "one");
    ASSERT_EQ(atoi("42"), 42, "simple number");
    ASSERT_EQ(atoi("123"), 123, "three digits");
    ASSERT_EQ(atoi("456789"), 456789, "six digits");
    ASSERT_EQ(atoi("2147483647"), 2147483647, "INT_MAX");

    printf("  PASSED\n");
    return true;
}

/**
 * Test atoi: negative numbers
 */
static inline bool stdlib_test_atoi_negative(void) {
    printf("\n[STDLIB TEST 2] atoi - Negative Numbers\n");

    ASSERT_EQ(atoi("-1"), -1, "minus one");
    ASSERT_EQ(atoi("-42"), -42, "negative number");
    ASSERT_EQ(atoi("-123"), -123, "negative three digits");
    ASSERT_EQ(atoi("-2147483648"), -2147483648, "INT_MIN");

    printf("  PASSED\n");
    return true;
}

/**
 * Test atoi: leading whitespace
 */
static inline bool stdlib_test_atoi_whitespace(void) {
    printf("\n[STDLIB TEST 3] atoi - Leading Whitespace\n");

    ASSERT_EQ(atoi(" 123"), 123, "single space");
    ASSERT_EQ(atoi("  456"), 456, "multiple spaces");
    ASSERT_EQ(atoi("\t789"), 789, "tab");
    ASSERT_EQ(atoi("\n42"), 42, "newline");
    ASSERT_EQ(atoi(" \t\n 99"), 99, "mixed whitespace");
    ASSERT_EQ(atoi("   -55"), -55, "whitespace before negative");

    printf("  PASSED\n");
    return true;
}

/**
 * Test atoi: leading zeros
 */
static inline bool stdlib_test_atoi_zeros(void) {
    printf("\n[STDLIB TEST 4] atoi - Leading Zeros\n");

    ASSERT_EQ(atoi("0"), 0, "single zero");
    ASSERT_EQ(atoi("00"), 0, "double zero");
    ASSERT_EQ(atoi("000123"), 123, "leading zeros");
    ASSERT_EQ(atoi("00042"), 42, "more leading zeros");
    ASSERT_EQ(atoi("-000123"), -123, "negative with leading zeros");

    printf("  PASSED\n");
    return true;
}

/**
 * Test atoi: invalid input
 */
static inline bool stdlib_test_atoi_invalid(void) {
    printf("\n[STDLIB TEST 5] atoi - Invalid Input\n");

    ASSERT_EQ(atoi("abc"), 0, "letters only");
    ASSERT_EQ(atoi(""), 0, "empty string");
    ASSERT_EQ(atoi("   "), 0, "whitespace only");
    ASSERT_EQ(atoi("123abc"), 123, "valid then invalid");
    ASSERT_EQ(atoi("12.34"), 12, "stops at decimal point");

    printf("  PASSED\n");
    return true;
}

/**
 * Test atoi: sign handling
 */
static inline bool stdlib_test_atoi_signs(void) {
    printf("\n[STDLIB TEST 6] atoi - Sign Handling\n");

    ASSERT_EQ(atoi("+123"), 123, "explicit plus sign");
    ASSERT_EQ(atoi("-123"), -123, "minus sign");
    ASSERT_EQ(atoi("+-123"), 0, "multiple signs (invalid)");
    ASSERT_EQ(atoi("  +456"), 456, "whitespace then plus");
    ASSERT_EQ(atoi("  -789"), -789, "whitespace then minus");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtol: basic functionality with base 10
 */
static inline bool stdlib_test_strtol_base10(void) {
    printf("\n[STDLIB TEST 7] strtol - Base 10\n");

    char* endptr;

    ASSERT_EQ(strtol("123", &endptr, 10), 123, "simple number");
    ASSERT_EQ(strtol("-456", &endptr, 10), -456, "negative number");
    ASSERT_EQ(strtol("0", &endptr, 10), 0, "zero");
    ASSERT_EQ(strtol("2147483647", &endptr, 10), 2147483647, "LONG_MAX");
    ASSERT_EQ(strtol("-2147483648", &endptr, 10), -2147483648, "LONG_MIN");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtol: endptr tracking
 */
static inline bool stdlib_test_strtol_endptr(void) {
    printf("\n[STDLIB TEST 8] strtol - End Pointer\n");

    char* endptr;
    const char* str1 = "123abc";

    strtol(str1, &endptr, 10);
    ASSERT_PTR_EQ(endptr, str1 + 3, "endptr points after digits");

    const char* str2 = "  456xyz";
    strtol(str2, &endptr, 10);
    ASSERT_PTR_EQ(endptr, str2 + 5, "endptr after whitespace and digits");

    const char* str3 = "abc";
    strtol(str3, &endptr, 10);
    ASSERT_PTR_EQ(endptr, str3, "endptr unchanged on no conversion");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtol: hexadecimal (base 16)
 */
static inline bool stdlib_test_strtol_hex(void) {
    printf("\n[STDLIB TEST 9] strtol - Hexadecimal\n");

    char* endptr;

    ASSERT_EQ(strtol("0x10", &endptr, 16), 0x10, "0x prefix");
    ASSERT_EQ(strtol("0X20", &endptr, 16), 0x20, "0X prefix");
    ASSERT_EQ(strtol("ff", &endptr, 16), 0xff, "lowercase hex digits");
    ASSERT_EQ(strtol("FF", &endptr, 16), 0xFF, "uppercase hex digits");
    ASSERT_EQ(strtol("DeadBeef", &endptr, 16), (long)0xDeadBeef, "mixed case");
    ASSERT_EQ(strtol("0xABCD", &endptr, 16), 0xABCD, "with prefix");
    ASSERT_EQ(strtol("1a2b", &endptr, 16), 0x1a2b, "without prefix");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtol: octal (base 8)
 */
static inline bool stdlib_test_strtol_octal(void) {
    printf("\n[STDLIB TEST 10] strtol - Octal\n");

    char* endptr;

    ASSERT_EQ(strtol("10", &endptr, 8), 8, "octal 10 = decimal 8");
    ASSERT_EQ(strtol("77", &endptr, 8), 63, "octal 77 = decimal 63");
    ASSERT_EQ(strtol("755", &endptr, 8), 493, "octal 755 = decimal 493");
    ASSERT_EQ(strtol("0", &endptr, 8), 0, "octal zero");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtol: binary (base 2)
 */
static inline bool stdlib_test_strtol_binary(void) {
    printf("\n[STDLIB TEST 11] strtol - Binary\n");

    char* endptr;

    ASSERT_EQ(strtol("0", &endptr, 2), 0, "binary 0");
    ASSERT_EQ(strtol("1", &endptr, 2), 1, "binary 1");
    ASSERT_EQ(strtol("10", &endptr, 2), 2, "binary 10");
    ASSERT_EQ(strtol("1010", &endptr, 2), 10, "binary 1010");
    ASSERT_EQ(strtol("11111111", &endptr, 2), 255, "binary 11111111");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtol: auto base detection (base 0)
 */
static inline bool stdlib_test_strtol_autobase(void) {
    printf("\n[STDLIB TEST 12] strtol - Auto Base Detection\n");

    char* endptr;

    ASSERT_EQ(strtol("123", &endptr, 0), 123, "decimal (no prefix)");
    ASSERT_EQ(strtol("0x10", &endptr, 0), 0x10, "hex with 0x");
    ASSERT_EQ(strtol("0X20", &endptr, 0), 0x20, "hex with 0X");
    ASSERT_EQ(strtol("077", &endptr, 0), 63, "octal with 0");
    ASSERT_EQ(strtol("0", &endptr, 0), 0, "zero");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtol: various bases
 */
static inline bool stdlib_test_strtol_bases(void) {
    printf("\n[STDLIB TEST 13] strtol - Various Bases\n");

    char* endptr;

    ASSERT_EQ(strtol("100", &endptr, 2), 4, "base 2");
    ASSERT_EQ(strtol("100", &endptr, 3), 9, "base 3");
    ASSERT_EQ(strtol("100", &endptr, 4), 16, "base 4");
    ASSERT_EQ(strtol("100", &endptr, 5), 25, "base 5");
    ASSERT_EQ(strtol("100", &endptr, 8), 64, "base 8");
    ASSERT_EQ(strtol("100", &endptr, 10), 100, "base 10");
    ASSERT_EQ(strtol("100", &endptr, 16), 256, "base 16");
    ASSERT_EQ(strtol("z", &endptr, 36), 35, "base 36 (max)");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtol: whitespace and signs
 */
static inline bool stdlib_test_strtol_whitespace(void) {
    printf("\n[STDLIB TEST 14] strtol - Whitespace and Signs\n");

    char* endptr;

    ASSERT_EQ(strtol("  123", &endptr, 10), 123, "leading spaces");
    ASSERT_EQ(strtol("\t456", &endptr, 10), 456, "leading tab");
    ASSERT_EQ(strtol("  +789", &endptr, 10), 789, "space then plus");
    ASSERT_EQ(strtol("  -999", &endptr, 10), -999, "space then minus");
    ASSERT_EQ(strtol(" \t\n +42", &endptr, 10), 42, "mixed whitespace");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtol: invalid input
 */
static inline bool stdlib_test_strtol_invalid(void) {
    printf("\n[STDLIB TEST 15] strtol - Invalid Input\n");

    char* endptr;
    const char* str;

    str = "abc";
    ASSERT_EQ(strtol(str, &endptr, 10), 0, "no digits");
    ASSERT_PTR_EQ(endptr, str, "endptr at start on no conversion");

    str = "0x";
    strtol(str, &endptr, 16);
    ASSERT_PTR_EQ(endptr, str, "0x without digits");

    str = "123g";
    ASSERT_EQ(strtol(str, &endptr, 16), 0x123, "stops at invalid hex digit");
    ASSERT_PTR_EQ(endptr, str + 3, "endptr before invalid char");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtoul: basic functionality
 */
static inline bool stdlib_test_strtoul_basic(void) {
    printf("\n[STDLIB TEST 16] strtoul - Basic\n");

    char* endptr;

    ASSERT_EQ(strtoul("123", &endptr, 10), 123UL, "simple number");
    ASSERT_EQ(strtoul("0", &endptr, 10), 0UL, "zero");
    ASSERT_EQ(strtoul("4294967295", &endptr, 10), 4294967295UL, "ULONG_MAX");
    ASSERT_EQ(strtoul("0xFF", &endptr, 16), 0xFFUL, "hex");
    ASSERT_EQ(strtoul("0777", &endptr, 0), 511UL, "octal auto-detect");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtoul: large unsigned values
 */
static inline bool stdlib_test_strtoul_large(void) {
    printf("\n[STDLIB TEST 17] strtoul - Large Values\n");

    char* endptr;

    ASSERT_EQ(strtoul("2147483648", &endptr, 10), 2147483648UL, "above INT_MAX");
    ASSERT_EQ(strtoul("3000000000", &endptr, 10), 3000000000UL, "3 billion");
    ASSERT_EQ(strtoul("4000000000", &endptr, 10), 4000000000UL, "4 billion");
    ASSERT_EQ(strtoul("FFFFFFFF", &endptr, 16), 0xFFFFFFFFUL, "max hex");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtoul: hex without prefix
 */
static inline bool stdlib_test_strtoul_hex(void) {
    printf("\n[STDLIB TEST 18] strtoul - Hexadecimal\n");

    char* endptr;

    ASSERT_EQ(strtoul("deadbeef", &endptr, 16), 0xdeadbeefUL, "lowercase hex");
    ASSERT_EQ(strtoul("DEADBEEF", &endptr, 16), 0xDEADBEEFUL, "uppercase hex");
    ASSERT_EQ(strtoul("0xCAFE", &endptr, 16), 0xCAFEUL, "with 0x prefix");
    ASSERT_EQ(strtoul("12AB", &endptr, 16), 0x12ABUL, "mixed digits and letters");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtoul: endptr tracking
 */
static inline bool stdlib_test_strtoul_endptr(void) {
    printf("\n[STDLIB TEST 19] strtoul - End Pointer\n");

    char* endptr;
    const char* str1 = "123xyz";

    strtoul(str1, &endptr, 10);
    ASSERT_PTR_EQ(endptr, str1 + 3, "endptr after valid digits");

    const char* str2 = "0xABCDEF123";
    strtoul(str2, &endptr, 0);
    ASSERT_PTR_EQ(endptr, str2 + 11, "endptr after hex number");

    const char* str3 = "  456more";
    strtoul(str3, &endptr, 10);
    ASSERT_PTR_EQ(endptr, str3 + 5, "endptr skips whitespace");

    printf("  PASSED\n");
    return true;
}

/**
 * Test strtoul: invalid bases
 */
static inline bool stdlib_test_strtoul_invalid_base(void) {
    printf("\n[STDLIB TEST 20] strtoul - Invalid Bases\n");

    char* endptr;
    const char* str = "123";

    ASSERT_EQ(strtoul(str, &endptr, -1), 0, "negative base");
    ASSERT_PTR_EQ(endptr, str, "endptr unchanged");

    ASSERT_EQ(strtoul(str, &endptr, 1), 0, "base 1");
    ASSERT_PTR_EQ(endptr, str, "endptr unchanged");

    ASSERT_EQ(strtoul(str, &endptr, 37), 0, "base 37 (too large)");
    ASSERT_PTR_EQ(endptr, str, "endptr unchanged");

    printf("  PASSED\n");
    return true;
}

/**
 * Run all stdlib tests
 */
static inline void stdlib_run_all_tests(void) {
    printf("\n========================================\n");
    printf("     Stdlib Library Test Suite\n");
    printf("========================================\n");

    int passed = 0;
    int total = 20;

    // atoi tests
    if (stdlib_test_atoi_positive()) passed++;
    if (stdlib_test_atoi_negative()) passed++;
    if (stdlib_test_atoi_whitespace()) passed++;
    if (stdlib_test_atoi_zeros()) passed++;
    if (stdlib_test_atoi_invalid()) passed++;
    if (stdlib_test_atoi_signs()) passed++;

    // strtol tests
    if (stdlib_test_strtol_base10()) passed++;
    if (stdlib_test_strtol_endptr()) passed++;
    if (stdlib_test_strtol_hex()) passed++;
    if (stdlib_test_strtol_octal()) passed++;
    if (stdlib_test_strtol_binary()) passed++;
    if (stdlib_test_strtol_autobase()) passed++;
    if (stdlib_test_strtol_bases()) passed++;
    if (stdlib_test_strtol_whitespace()) passed++;
    if (stdlib_test_strtol_invalid()) passed++;

    // strtoul tests
    if (stdlib_test_strtoul_basic()) passed++;
    if (stdlib_test_strtoul_large()) passed++;
    if (stdlib_test_strtoul_hex()) passed++;
    if (stdlib_test_strtoul_endptr()) passed++;
    if (stdlib_test_strtoul_invalid_base()) passed++;

    // Print results
    printf("\n========================================\n");
    printf("  Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");

    if (passed == total) {
        printf("\n[OK] All stdlib tests PASSED!\n\n");
    } else {
        printf("\n[FAIL] Some stdlib tests FAILED\n\n");
    }
}

#endif // KERNEL_LIBC_STDLIB_TESTS_H
