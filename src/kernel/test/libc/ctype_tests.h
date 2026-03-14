#ifndef KERNEL_LIBC_CTYPE_TESTS_H
#define KERNEL_LIBC_CTYPE_TESTS_H

#include <ctype.h>
#include <stdio.h>
#include "test_base.h"

/**
 * Ctype Library Test Suite
 *
 * Comprehensive tests for all ctype.h character classification functions
 */

/**
 * Test isdigit: digit characters
 */
static inline bool ctype_test_isdigit(void) {
    printf("\n[CTYPE TEST 1] isdigit\n");

    // Valid digits
    for (char c = '0'; c <= '9'; c++) {
        ASSERT_TRUE(isdigit(c), "digit character");
    }

    // Invalid - letters
    ASSERT_FALSE(isdigit('a'), "lowercase letter");
    ASSERT_FALSE(isdigit('Z'), "uppercase letter");

    // Invalid - special characters
    ASSERT_FALSE(isdigit(' '), "space");
    ASSERT_FALSE(isdigit('/'), "slash (before '0')");
    ASSERT_FALSE(isdigit(':'), "colon (after '9')");
    ASSERT_FALSE(isdigit('\t'), "tab");
    ASSERT_FALSE(isdigit('\n'), "newline");

    printf("  PASSED\n");
    return true;
}

/**
 * Test isalpha: alphabetic characters
 */
static inline bool ctype_test_isalpha(void) {
    printf("\n[CTYPE TEST 2] isalpha\n");

    // Valid - lowercase
    for (char c = 'a'; c <= 'z'; c++) {
        ASSERT_TRUE(isalpha(c), "lowercase letter");
    }

    // Valid - uppercase
    for (char c = 'A'; c <= 'Z'; c++) {
        ASSERT_TRUE(isalpha(c), "uppercase letter");
    }

    // Invalid - digits
    ASSERT_FALSE(isalpha('0'), "digit");
    ASSERT_FALSE(isalpha('9'), "digit");

    // Invalid - special
    ASSERT_FALSE(isalpha(' '), "space");
    ASSERT_FALSE(isalpha('@'), "at sign (before 'A')");
    ASSERT_FALSE(isalpha('['), "bracket (after 'Z')");
    ASSERT_FALSE(isalpha('`'), "backtick (before 'a')");
    ASSERT_FALSE(isalpha('{'), "brace (after 'z')");

    printf("  PASSED\n");
    return true;
}

/**
 * Test isupper: uppercase letters
 */
static inline bool ctype_test_isupper(void) {
    printf("\n[CTYPE TEST 3] isupper\n");

    // Valid uppercase
    for (char c = 'A'; c <= 'Z'; c++) {
        ASSERT_TRUE(isupper(c), "uppercase letter");
    }

    // Invalid - lowercase
    for (char c = 'a'; c <= 'z'; c++) {
        ASSERT_FALSE(isupper(c), "lowercase letter");
    }

    // Invalid - digits and special
    ASSERT_FALSE(isupper('0'), "digit");
    ASSERT_FALSE(isupper(' '), "space");
    ASSERT_FALSE(isupper('@'), "at sign");

    printf("  PASSED\n");
    return true;
}

/**
 * Test islower: lowercase letters
 */
static inline bool ctype_test_islower(void) {
    printf("\n[CTYPE TEST 4] islower\n");

    // Valid lowercase
    for (char c = 'a'; c <= 'z'; c++) {
        ASSERT_TRUE(islower(c), "lowercase letter");
    }

    // Invalid - uppercase
    for (char c = 'A'; c <= 'Z'; c++) {
        ASSERT_FALSE(islower(c), "uppercase letter");
    }

    // Invalid - digits and special
    ASSERT_FALSE(islower('0'), "digit");
    ASSERT_FALSE(islower(' '), "space");
    ASSERT_FALSE(islower('`'), "backtick");

    printf("  PASSED\n");
    return true;
}

/**
 * Test isalnum: alphanumeric characters
 */
static inline bool ctype_test_isalnum(void) {
    printf("\n[CTYPE TEST 5] isalnum\n");

    // Valid - digits
    for (char c = '0'; c <= '9'; c++) {
        ASSERT_TRUE(isalnum(c), "digit");
    }

    // Valid - lowercase letters
    for (char c = 'a'; c <= 'z'; c++) {
        ASSERT_TRUE(isalnum(c), "lowercase letter");
    }

    // Valid - uppercase letters
    for (char c = 'A'; c <= 'Z'; c++) {
        ASSERT_TRUE(isalnum(c), "uppercase letter");
    }

    // Invalid - special characters
    ASSERT_FALSE(isalnum(' '), "space");
    ASSERT_FALSE(isalnum('!'), "exclamation");
    ASSERT_FALSE(isalnum('@'), "at sign");
    ASSERT_FALSE(isalnum('['), "bracket");
    ASSERT_FALSE(isalnum('_'), "underscore");

    printf("  PASSED\n");
    return true;
}

/**
 * Test isxdigit: hexadecimal digits
 */
static inline bool ctype_test_isxdigit(void) {
    printf("\n[CTYPE TEST 6] isxdigit\n");

    // Valid - decimal digits
    for (char c = '0'; c <= '9'; c++) {
        ASSERT_TRUE(isxdigit(c), "decimal digit");
    }

    // Valid - lowercase hex
    for (char c = 'a'; c <= 'f'; c++) {
        ASSERT_TRUE(isxdigit(c), "lowercase hex digit");
    }

    // Valid - uppercase hex
    for (char c = 'A'; c <= 'F'; c++) {
        ASSERT_TRUE(isxdigit(c), "uppercase hex digit");
    }

    // Invalid - letters beyond F
    ASSERT_FALSE(isxdigit('g'), "g");
    ASSERT_FALSE(isxdigit('G'), "G");
    ASSERT_FALSE(isxdigit('z'), "z");
    ASSERT_FALSE(isxdigit('Z'), "Z");

    // Invalid - special characters
    ASSERT_FALSE(isxdigit(' '), "space");
    ASSERT_FALSE(isxdigit('@'), "at sign");

    printf("  PASSED\n");
    return true;
}

/**
 * Test isspace: whitespace characters
 */
static inline bool ctype_test_isspace(void) {
    printf("\n[CTYPE TEST 7] isspace\n");

    // Valid whitespace
    ASSERT_TRUE(isspace(' '), "space");
    ASSERT_TRUE(isspace('\t'), "tab");
    ASSERT_TRUE(isspace('\n'), "newline");
    ASSERT_TRUE(isspace('\r'), "carriage return");
    ASSERT_TRUE(isspace('\f'), "form feed");
    ASSERT_TRUE(isspace('\v'), "vertical tab");

    // Invalid
    ASSERT_FALSE(isspace('a'), "letter");
    ASSERT_FALSE(isspace('0'), "digit");
    ASSERT_FALSE(isspace('_'), "underscore");
    ASSERT_FALSE(isspace('\0'), "null terminator");

    printf("  PASSED\n");
    return true;
}

/**
 * Test isblank: blank characters (space and tab)
 */
static inline bool ctype_test_isblank(void) {
    printf("\n[CTYPE TEST 8] isblank\n");

    // Valid blanks
    ASSERT_TRUE(isblank(' '), "space");
    ASSERT_TRUE(isblank('\t'), "tab");

    // Invalid - other whitespace
    ASSERT_FALSE(isblank('\n'), "newline");
    ASSERT_FALSE(isblank('\r'), "carriage return");
    ASSERT_FALSE(isblank('\f'), "form feed");
    ASSERT_FALSE(isblank('\v'), "vertical tab");

    // Invalid - other characters
    ASSERT_FALSE(isblank('a'), "letter");
    ASSERT_FALSE(isblank('0'), "digit");

    printf("  PASSED\n");
    return true;
}

/**
 * Test ispunct: punctuation characters
 */
static inline bool ctype_test_ispunct(void) {
    printf("\n[CTYPE TEST 9] ispunct\n");

    // Valid punctuation
    ASSERT_TRUE(ispunct('!'), "exclamation");
    ASSERT_TRUE(ispunct('"'), "quote");
    ASSERT_TRUE(ispunct('#'), "hash");
    ASSERT_TRUE(ispunct('$'), "dollar");
    ASSERT_TRUE(ispunct('%'), "percent");
    ASSERT_TRUE(ispunct('&'), "ampersand");
    ASSERT_TRUE(ispunct('\''), "apostrophe");
    ASSERT_TRUE(ispunct('('), "left paren");
    ASSERT_TRUE(ispunct(')'), "right paren");
    ASSERT_TRUE(ispunct('*'), "asterisk");
    ASSERT_TRUE(ispunct('+'), "plus");
    ASSERT_TRUE(ispunct(','), "comma");
    ASSERT_TRUE(ispunct('-'), "minus");
    ASSERT_TRUE(ispunct('.'), "period");
    ASSERT_TRUE(ispunct('/'), "slash");
    ASSERT_TRUE(ispunct(':'), "colon");
    ASSERT_TRUE(ispunct(';'), "semicolon");
    ASSERT_TRUE(ispunct('<'), "less than");
    ASSERT_TRUE(ispunct('='), "equals");
    ASSERT_TRUE(ispunct('>'), "greater than");
    ASSERT_TRUE(ispunct('?'), "question");
    ASSERT_TRUE(ispunct('@'), "at sign");
    ASSERT_TRUE(ispunct('['), "left bracket");
    ASSERT_TRUE(ispunct('\\'), "backslash");
    ASSERT_TRUE(ispunct(']'), "right bracket");
    ASSERT_TRUE(ispunct('^'), "caret");
    ASSERT_TRUE(ispunct('_'), "underscore");
    ASSERT_TRUE(ispunct('`'), "backtick");
    ASSERT_TRUE(ispunct('{'), "left brace");
    ASSERT_TRUE(ispunct('|'), "pipe");
    ASSERT_TRUE(ispunct('}'), "right brace");
    ASSERT_TRUE(ispunct('~'), "tilde");

    // Invalid - letters, digits, space
    ASSERT_FALSE(ispunct('a'), "letter");
    ASSERT_FALSE(ispunct('Z'), "letter");
    ASSERT_FALSE(ispunct('0'), "digit");
    ASSERT_FALSE(ispunct('9'), "digit");
    ASSERT_FALSE(ispunct(' '), "space");

    printf("  PASSED\n");
    return true;
}

/**
 * Test isgraph: graphical characters
 */
static inline bool ctype_test_isgraph(void) {
    printf("\n[CTYPE TEST 10] isgraph\n");

    // Valid - all printable non-space characters
    for (char c = '!'; c <= '~'; c++) {
        ASSERT_TRUE(isgraph(c), "graphical character");
    }

    // Invalid - space and control characters
    ASSERT_FALSE(isgraph(' '), "space");
    ASSERT_FALSE(isgraph('\t'), "tab");
    ASSERT_FALSE(isgraph('\n'), "newline");
    ASSERT_FALSE(isgraph('\0'), "null");
    ASSERT_FALSE(isgraph('\x01'), "control character");

    printf("  PASSED\n");
    return true;
}

/**
 * Test isprint: printable characters
 */
static inline bool ctype_test_isprint(void) {
    printf("\n[CTYPE TEST 11] isprint\n");

    // Valid - space through tilde
    for (char c = ' '; c <= '~'; c++) {
        ASSERT_TRUE(isprint(c), "printable character");
    }

    // Invalid - control characters
    ASSERT_FALSE(isprint('\0'), "null");
    ASSERT_FALSE(isprint('\t'), "tab");
    ASSERT_FALSE(isprint('\n'), "newline");
    ASSERT_FALSE(isprint('\r'), "carriage return");
    ASSERT_FALSE(isprint('\x01'), "SOH");
    ASSERT_FALSE(isprint('\x1F'), "US (unit separator)");
    ASSERT_FALSE(isprint('\x7F'), "DEL");

    printf("  PASSED\n");
    return true;
}

/**
 * Test iscntrl: control characters
 */
static inline bool ctype_test_iscntrl(void) {
    printf("\n[CTYPE TEST 12] iscntrl\n");

    // Valid - control characters 0x00-0x1F
    for (int c = 0; c < 0x20; c++) {
        ASSERT_TRUE(iscntrl(c), "control character");
    }

    // Valid - DEL (0x7F)
    ASSERT_TRUE(iscntrl('\x7F'), "DEL character");

    // Invalid - printable characters
    ASSERT_FALSE(iscntrl(' '), "space");
    ASSERT_FALSE(iscntrl('a'), "letter");
    ASSERT_FALSE(iscntrl('0'), "digit");
    ASSERT_FALSE(iscntrl('!'), "punctuation");

    printf("  PASSED\n");
    return true;
}

/**
 * Test boundary conditions and edge cases
 */
static inline bool ctype_test_boundaries(void) {
    printf("\n[CTYPE TEST 13] Boundary Conditions\n");

    // Test ASCII boundaries
    ASSERT_TRUE(iscntrl(0x00), "0x00 is control");
    ASSERT_TRUE(iscntrl(0x1F), "0x1F is control");
    ASSERT_TRUE(isprint(0x20), "0x20 (space) is printable");
    ASSERT_TRUE(isprint(0x7E), "0x7E (~) is printable");
    ASSERT_TRUE(iscntrl(0x7F), "0x7F is control");

    // Test digit boundaries
    ASSERT_FALSE(isdigit('/'), "/ (0x2F, before '0')");
    ASSERT_TRUE(isdigit('0'), "'0' (0x30)");
    ASSERT_TRUE(isdigit('9'), "'9' (0x39)");
    ASSERT_FALSE(isdigit(':'), ": (0x3A, after '9')");

    // Test uppercase boundaries
    ASSERT_FALSE(isupper('@'), "@ (0x40, before 'A')");
    ASSERT_TRUE(isupper('A'), "'A' (0x41)");
    ASSERT_TRUE(isupper('Z'), "'Z' (0x5A)");
    ASSERT_FALSE(isupper('['), "[ (0x5B, after 'Z')");

    // Test lowercase boundaries
    ASSERT_FALSE(islower('`'), "` (0x60, before 'a')");
    ASSERT_TRUE(islower('a'), "'a' (0x61)");
    ASSERT_TRUE(islower('z'), "'z' (0x7A)");
    ASSERT_FALSE(islower('{'), "{ (0x7B, after 'z')");

    printf("  PASSED\n");
    return true;
}

/**
 * Run all ctype tests
 */
static inline void ctype_run_all_tests(void) {
    printf("\n=== Ctype Library Tests ===\n");
    int passed = 0;
    int total = 13;

    if (ctype_test_isdigit()) passed++;
    if (ctype_test_isalpha()) passed++;
    if (ctype_test_isupper()) passed++;
    if (ctype_test_islower()) passed++;
    if (ctype_test_isalnum()) passed++;
    if (ctype_test_isxdigit()) passed++;
    if (ctype_test_isspace()) passed++;
    if (ctype_test_isblank()) passed++;
    if (ctype_test_ispunct()) passed++;
    if (ctype_test_isgraph()) passed++;
    if (ctype_test_isprint()) passed++;
    if (ctype_test_iscntrl()) passed++;
    if (ctype_test_boundaries()) passed++;

    // Print results
    if (passed == total) {
        printf("Ctype: %d/%d PASSED\n", passed, total);
    } else {
        printf("Ctype: %d/%d FAILED\n", passed, total);
    }
}

#endif // KERNEL_LIBC_CTYPE_TESTS_H
