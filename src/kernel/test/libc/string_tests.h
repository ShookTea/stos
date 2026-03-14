#ifndef KERNEL_LIBC_STRING_TESTS_H
#define KERNEL_LIBC_STRING_TESTS_H

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "test_base.h"

/**
 * String Library Test Suite
 *
 * Comprehensive tests for all string.h functions
 */

/**
 * Test strlen: basic functionality
 */
static inline bool string_test_strlen_basic(void) {
    ASSERT_EQ(strlen(""), 0, "empty string");
    ASSERT_EQ(strlen("a"), 1, "single character");
    ASSERT_EQ(strlen("hello"), 5, "normal string");
    ASSERT_EQ(strlen("hello world"), 11, "string with space");
    ASSERT_EQ(strlen("The quick brown fox jumps over the lazy dog"), 43, "long string");
    return true;
}

/**
 * Test strlen: special characters
 */
static inline bool string_test_strlen_special(void) {
    ASSERT_EQ(strlen("\n"), 1, "newline");
    ASSERT_EQ(strlen("\t"), 1, "tab");
    ASSERT_EQ(strlen("hello\nworld"), 11, "string with newline");
    ASSERT_EQ(strlen("\x01\x02\x03"), 3, "binary characters");
    return true;
}

/**
 * Test strcmp: equality
 */
static inline bool string_test_strcmp_equal(void) {
    ASSERT_EQ(strcmp("", ""), 0, "empty strings equal");
    ASSERT_EQ(strcmp("a", "a"), 0, "single char equal");
    ASSERT_EQ(strcmp("hello", "hello"), 0, "identical strings");
    ASSERT_EQ(strcmp("test123", "test123"), 0, "alphanumeric equal");
    return true;
}

/**
 * Test strcmp: ordering
 */
static inline bool string_test_strcmp_ordering(void) {
    ASSERT_TRUE(strcmp("a", "b") < 0, "a < b");
    ASSERT_TRUE(strcmp("b", "a") > 0, "b > a");
    ASSERT_TRUE(strcmp("abc", "abd") < 0, "abc < abd");
    ASSERT_TRUE(strcmp("abc", "abb") > 0, "abc > abb");
    ASSERT_TRUE(strcmp("abc", "abcd") < 0, "abc < abcd (shorter)");
    ASSERT_TRUE(strcmp("abcd", "abc") > 0, "abcd > abc (longer)");
    ASSERT_TRUE(strcmp("ABC", "abc") < 0, "uppercase < lowercase in ASCII");
    return true;
}

/**
 * Test strncmp: basic functionality
 */
static inline bool string_test_strncmp_basic(void) {
    ASSERT_EQ(strncmp("hello", "hello", 5), 0, "equal strings, full length");
    ASSERT_EQ(strncmp("hello", "help", 3), 0, "equal first 3 chars");
    ASSERT_TRUE(strncmp("hello", "help", 4) != 0, "different at 4th char");
    ASSERT_EQ(strncmp("test", "testing", 4), 0, "prefix match");
    ASSERT_EQ(strncmp("", "", 0), 0, "zero length comparison");
    ASSERT_EQ(strncmp("abc", "xyz", 0), 0, "zero length always equal");
    return true;
}

/**
 * Test strncmp: boundary conditions
 */
static inline bool string_test_strncmp_boundary(void) {
    ASSERT_EQ(strncmp("hello", "hello", 10), 0, "n > string length");
    ASSERT_EQ(strncmp("abc", "abc", 100), 0, "n much larger than length");
    ASSERT_TRUE(strncmp("abc", "abd", 3) < 0, "difference at last char");
    ASSERT_TRUE(strncmp("abd", "abc", 3) > 0, "difference at last char (reverse)");
    return true;
}

/**
 * Test memcmp: basic functionality
 */
static inline bool string_test_memcmp_basic(void) {
    char buf1[] = "hello";
    char buf2[] = "hello";
    char buf3[] = "help!";

    ASSERT_EQ(memcmp(buf1, buf2, 5), 0, "equal buffers");
    ASSERT_TRUE(memcmp(buf1, buf3, 5) != 0, "different buffers");
    ASSERT_EQ(memcmp(buf1, buf3, 3), 0, "equal first 3 bytes");
    ASSERT_TRUE(memcmp(buf1, buf3, 4) < 0, "buf1 < buf3 at 4th byte");
    return true;
}

/**
 * Test memcmp: with null bytes
 */
static inline bool string_test_memcmp_nulls(void) {
    char buf1[] = {'a', '\0', 'b', 'c'};
    char buf2[] = {'a', '\0', 'b', 'c'};
    char buf3[] = {'a', '\0', 'x', 'c'};

    ASSERT_EQ(memcmp(buf1, buf2, 4), 0, "buffers with null equal");
    ASSERT_TRUE(memcmp(buf1, buf3, 4) != 0, "buffers with null different");
    ASSERT_EQ(memcmp(buf1, buf3, 2), 0, "equal including null");
    ASSERT_TRUE(memcmp(buf1, buf3, 3) < 0, "difference after null");
    return true;
}

/**
 * Test memcpy: basic functionality
 */
static inline bool string_test_memcpy_basic(void) {
    char src[] = "hello world";
    char dst[20];

    memcpy(dst, src, 12);  // including null terminator
    ASSERT_STR_EQ(dst, "hello world", "basic string copy");

    char src2[] = {1, 2, 3, 4, 5};
    char dst2[5];
    memcpy(dst2, src2, 5);
    
    bool binary_match = true;
    for (int i = 0; i < 5; i++) {
        if (dst2[i] != src2[i]) {
            binary_match = false;
            break;
        }
    }
    ASSERT_TRUE(binary_match, "binary data copy");
    return true;
}

/**
 * Test memcpy: various sizes
 */
static inline bool string_test_memcpy_sizes(void) {
    // Test different sizes
    char src[256];
    char dst[256];

    // Fill source with pattern
    for (int i = 0; i < 256; i++) {
        src[i] = (char)(i & 0xFF);
    }

    // Test size 1
    memcpy(dst, src, 1);
    ASSERT_EQ(dst[0], 0, "size 1 copy");

    // Test size 16
    memcpy(dst, src, 16);
    bool match_16 = true;
    for (int i = 0; i < 16; i++) {
        if (dst[i] != src[i]) {
            match_16 = false;
            break;
        }
    }
    ASSERT_TRUE(match_16, "size 16 copy");

    // Test size 256
    memcpy(dst, src, 256);
    bool match_256 = true;
    for (int i = 0; i < 256; i++) {
        if (dst[i] != src[i]) {
            match_256 = false;
            break;
        }
    }
    ASSERT_TRUE(match_256, "size 256 copy");
    return true;
}

/**
 * Test memset: basic functionality
 */
static inline bool string_test_memset_basic(void) {
    char buf[20];

    memset(buf, 0, 20);
    bool all_zero = true;
    for (int i = 0; i < 20; i++) {
        if (buf[i] != 0) {
            all_zero = false;
            break;
        }
    }
    ASSERT_TRUE(all_zero, "set to zero");

    memset(buf, 'A', 10);
    bool first_ten_a = true;
    for (int i = 0; i < 10; i++) {
        if (buf[i] != 'A') {
            first_ten_a = false;
            break;
        }
    }
    ASSERT_TRUE(first_ten_a, "set first 10 to 'A'");
    return true;
}

/**
 * Test memset: various patterns
 */
static inline bool string_test_memset_patterns(void) {
    char buf[16];

    // Test with 0xFF
    memset(buf, 0xFF, 16);
    bool all_ff = true;
    for (int i = 0; i < 16; i++) {
        if ((unsigned char)buf[i] != 0xFF) {
            all_ff = false;
            break;
        }
    }
    ASSERT_TRUE(all_ff, "set to 0xFF");

    // Test with 0x55 (01010101 pattern)
    memset(buf, 0x55, 16);
    bool all_55 = true;
    for (int i = 0; i < 16; i++) {
        if ((unsigned char)buf[i] != 0x55) {
            all_55 = false;
            break;
        }
    }
    ASSERT_TRUE(all_55, "set to 0x55");
    return true;
}

/**
 * Test memmove: non-overlapping
 */
static inline bool string_test_memmove_nonoverlap(void) {
    char buf[20] = "hello world";
    char dst[20];

    memmove(dst, buf, 12);
    ASSERT_STR_EQ(dst, "hello world", "non-overlapping move");
    return true;
}

/**
 * Test memmove: overlapping regions
 */
static inline bool string_test_memmove_overlap(void) {
    char buf1[20] = "hello world";
    // Move forward: src overlaps with dst (dst > src)
    memmove(buf1 + 2, buf1, 11);
    ASSERT_STR_EQ(buf1 + 2, "hello world", "forward overlap");

    char buf2[20] = "hello world";
    // Move backward: dst overlaps with src (src > dst)
    memmove(buf2, buf2 + 6, 5);
    buf2[5] = '\0';  // null terminate
    ASSERT_STR_EQ(buf2, "world", "backward overlap");
    return true;
}

/**
 * Test strcpy: basic functionality
 */
static inline bool string_test_strcpy_basic(void) {
    char dst[20];

    strcpy(dst, "hello");
    ASSERT_STR_EQ(dst, "hello", "basic copy");

    strcpy(dst, "");
    ASSERT_STR_EQ(dst, "", "empty string copy");

    strcpy(dst, "a");
    ASSERT_STR_EQ(dst, "a", "single character");
    return true;
}

/**
 * Test strcat: basic functionality
 */
static inline bool string_test_strcat_basic(void) {
    char dst[50] = "hello";
    
    strcat(dst, " world");
    ASSERT_STR_EQ(dst, "hello world", "basic concatenation");

    strcat(dst, "!");
    ASSERT_STR_EQ(dst, "hello world!", "append single char");

    char dst2[50] = "";
    strcat(dst2, "test");
    ASSERT_STR_EQ(dst2, "test", "concatenate to empty");
    return true;
}

/**
 * Test strcat: multiple concatenations
 */
static inline bool string_test_strcat_multiple(void) {
    char dst[100] = "The";
    strcat(dst, " quick");
    strcat(dst, " brown");
    strcat(dst, " fox");
    
    ASSERT_STR_EQ(dst, "The quick brown fox", "multiple concatenations");
    return true;
}

/**
 * Test strchr: basic functionality
 */
static inline bool string_test_strchr_basic(void) {
    const char* str = "hello world";
    
    char* result = strchr(str, 'h');
    ASSERT_PTR_EQ(result, str, "find first character");

    result = strchr(str, 'o');
    ASSERT_PTR_EQ(result, str + 4, "find 'o' (first occurrence)");

    result = strchr(str, 'd');
    ASSERT_PTR_EQ(result, str + 10, "find last character");

    result = strchr(str, '\0');
    ASSERT_PTR_EQ(result, str + 11, "find null terminator");

    result = strchr(str, 'x');
    ASSERT_PTR_EQ(result, NULL, "character not found");
    return true;
}

/**
 * Test strchr: edge cases
 */
static inline bool string_test_strchr_edge(void) {
    const char* empty = "";
    char* result = strchr(empty, 'a');
    ASSERT_PTR_EQ(result, NULL, "search in empty string");

    result = strchr(empty, '\0');
    ASSERT_PTR_EQ(result, empty, "find null in empty string");

    const char* single = "a";
    result = strchr(single, 'a');
    ASSERT_PTR_EQ(result, single, "find in single char string");
    return true;
}

/**
 * Test strspn: basic functionality
 */
static inline bool string_test_strspn_basic(void) {
    ASSERT_EQ(strspn("hello", "helo"), 5, "all chars match");
    ASSERT_EQ(strspn("hello", "xyz"), 0, "no chars match");
    ASSERT_EQ(strspn("hello world", "helo "), 6, "partial match");
    ASSERT_EQ(strspn("123abc", "0123456789"), 3, "match digits");
    ASSERT_EQ(strspn("", "abc"), 0, "empty string");
    ASSERT_EQ(strspn("abc", ""), 0, "empty accept");
    return true;
}

/**
 * Test strspn: whitespace
 */
static inline bool string_test_strspn_whitespace(void) {
    ASSERT_EQ(strspn("   hello", " \t\n"), 3, "leading spaces");
    ASSERT_EQ(strspn("\t\t\ttest", " \t\n"), 3, "leading tabs");
    ASSERT_EQ(strspn(" \t\n\r  abc", " \t\n\r"), 6, "mixed whitespace");
    return true;
}

/**
 * Test strtok: basic functionality
 */
static inline bool string_test_strtok_basic(void) {
    char str[] = "hello,world,test";
    char* token;

    token = strtok(str, ",");
    ASSERT_STR_EQ(token, "hello", "first token");

    token = strtok(NULL, ",");
    ASSERT_STR_EQ(token, "world", "second token");

    token = strtok(NULL, ",");
    ASSERT_STR_EQ(token, "test", "third token");

    token = strtok(NULL, ",");
    ASSERT_PTR_EQ(token, NULL, "no more tokens");
    return true;
}

/**
 * Test strtok: multiple delimiters
 */
static inline bool string_test_strtok_multidelim(void) {
    char str[] = "one:two;three:four";
    char* token;

    token = strtok(str, ":;");
    ASSERT_STR_EQ(token, "one", "first token");

    token = strtok(NULL, ":;");
    ASSERT_STR_EQ(token, "two", "second token");

    token = strtok(NULL, ":;");
    ASSERT_STR_EQ(token, "three", "third token");

    token = strtok(NULL, ":;");
    ASSERT_STR_EQ(token, "four", "fourth token");

    token = strtok(NULL, ":;");
    ASSERT_PTR_EQ(token, NULL, "no more tokens");
    return true;
}

/**
 * Test strtok: consecutive delimiters
 */
static inline bool string_test_strtok_consecutive(void) {
    char str[] = "one,,three";
    char* token;

    token = strtok(str, ",");
    ASSERT_STR_EQ(token, "one", "first token");

    token = strtok(NULL, ",");
    ASSERT_STR_EQ(token, "three", "second token (skips empty)");

    token = strtok(NULL, ",");
    ASSERT_PTR_EQ(token, NULL, "no more tokens");
    return true;
}

/**
 * Test strtok: leading and trailing delimiters
 */
static inline bool string_test_strtok_edges(void) {
    char str[] = ",,,hello,world,,,";
    char* token;

    token = strtok(str, ",");
    ASSERT_STR_EQ(token, "hello", "first token (skip leading)");

    token = strtok(NULL, ",");
    ASSERT_STR_EQ(token, "world", "second token");

    token = strtok(NULL, ",");
    ASSERT_PTR_EQ(token, NULL, "no more tokens (skip trailing)");
    return true;
}

/**
 * Run all string tests
 */
static inline void string_run_all_tests(void) {
    printf("\n=== String Library Tests ===\n");
    int passed = 0;
    int total = 25;

    // strlen tests
    if (string_test_strlen_basic()) passed++;
    if (string_test_strlen_special()) passed++;

    // strcmp tests
    if (string_test_strcmp_equal()) passed++;
    if (string_test_strcmp_ordering()) passed++;

    // strncmp tests
    if (string_test_strncmp_basic()) passed++;
    if (string_test_strncmp_boundary()) passed++;

    // memcmp tests
    if (string_test_memcmp_basic()) passed++;
    if (string_test_memcmp_nulls()) passed++;

    // memcpy tests
    if (string_test_memcpy_basic()) passed++;
    if (string_test_memcpy_sizes()) passed++;

    // memset tests
    if (string_test_memset_basic()) passed++;
    if (string_test_memset_patterns()) passed++;

    // memmove tests
    if (string_test_memmove_nonoverlap()) passed++;
    if (string_test_memmove_overlap()) passed++;

    // strcpy tests
    if (string_test_strcpy_basic()) passed++;

    // strcat tests
    if (string_test_strcat_basic()) passed++;
    if (string_test_strcat_multiple()) passed++;

    // strchr tests
    if (string_test_strchr_basic()) passed++;
    if (string_test_strchr_edge()) passed++;

    // strspn tests
    if (string_test_strspn_basic()) passed++;
    if (string_test_strspn_whitespace()) passed++;

    // strtok tests
    if (string_test_strtok_basic()) passed++;
    if (string_test_strtok_multidelim()) passed++;
    if (string_test_strtok_consecutive()) passed++;
    if (string_test_strtok_edges()) passed++;

    // Print results
    if (passed == total) {
        printf("String: %d/%d PASSED\n", passed, total);
    } else {
        printf("String: %d/%d FAILED\n", passed, total);
    }
}

#endif // KERNEL_LIBC_STRING_TESTS_H
