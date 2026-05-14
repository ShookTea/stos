#ifndef KERNEL_LIBC_GETOPT_TESTS_H
#define KERNEL_LIBC_GETOPT_TESTS_H

#include <unistd.h>
#include <getopt.h>
#include <stddef.h>
#include "kernel/debug.h"
#include "test_base.h"

/* Reset getopt global state between tests */
#define GETOPT_RESET() do { optind = 1; opterr = 0; } while(0)

/* ========== getopt tests ========== */

static inline bool getopt_test_basic_option(void) {
    char* argv[] = {"prog", "-a", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "a"), 'a', "recognized option 'a'");
    ASSERT_EQ(getopt(argc, argv, "a"), -1, "returns -1 when done");
    return true;
}

static inline bool getopt_test_multiple_options(void) {
    char* argv[] = {"prog", "-a", "-b", "-c", NULL};
    int argc = 4;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "abc"), 'a', "first option");
    ASSERT_EQ(getopt(argc, argv, "abc"), 'b', "second option");
    ASSERT_EQ(getopt(argc, argv, "abc"), 'c', "third option");
    ASSERT_EQ(getopt(argc, argv, "abc"), -1, "done");
    return true;
}

static inline bool getopt_test_combined_options(void) {
    char* argv[] = {"prog", "-abc", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "abc"), 'a', "combined: first");
    ASSERT_EQ(getopt(argc, argv, "abc"), 'b', "combined: second");
    ASSERT_EQ(getopt(argc, argv, "abc"), 'c', "combined: third");
    ASSERT_EQ(getopt(argc, argv, "abc"), -1, "combined: done");
    return true;
}

static inline bool getopt_test_required_arg_inline(void) {
    char* argv[] = {"prog", "-oFILE", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "o:"), 'o', "option 'o'");
    ASSERT_STR_EQ(optarg, "FILE", "inline argument");
    ASSERT_EQ(getopt(argc, argv, "o:"), -1, "done");
    return true;
}

static inline bool getopt_test_required_arg_next(void) {
    char* argv[] = {"prog", "-o", "FILE", NULL};
    int argc = 3;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "o:"), 'o', "option 'o'");
    ASSERT_STR_EQ(optarg, "FILE", "next-element argument");
    ASSERT_EQ(getopt(argc, argv, "o:"), -1, "done");
    return true;
}

static inline bool getopt_test_optional_arg_present(void) {
    char* argv[] = {"prog", "-oFILE", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "o::"), 'o', "option with optional arg present");
    ASSERT_STR_EQ(optarg, "FILE", "optional arg present inline");
    return true;
}

static inline bool getopt_test_optional_arg_absent(void) {
    char* argv[] = {"prog", "-o", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "o::"), 'o', "option with optional arg absent");
    ASSERT_NULL(optarg, "optional arg absent: optarg is NULL");
    return true;
}

static inline bool getopt_test_optional_arg_not_next_element(void) {
    /* optional arg is NOT taken from the next argv element, only inline */
    char* argv[] = {"prog", "-o", "FILE", NULL};
    int argc = 3;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "o::"), 'o', "option with optional arg");
    ASSERT_NULL(optarg, "optional: next element is not consumed");
    /* 'FILE' remains as a non-option */
    ASSERT_EQ(getopt(argc, argv, "o::"), -1, "done");
    return true;
}

static inline bool getopt_test_no_options(void) {
    char* argv[] = {"prog", "file1", "file2", NULL};
    int argc = 3;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "a"), -1, "no options found");
    ASSERT_EQ(optind, 1, "optind at first non-option");
    return true;
}

static inline bool getopt_test_double_dash(void) {
    char* argv[] = {"prog", "-a", "--", "-b", NULL};
    int argc = 4;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "ab"), 'a', "option before --");
    ASSERT_EQ(getopt(argc, argv, "ab"), -1, "-- terminates option processing");
    ASSERT_EQ(optind, 3, "optind points past --");
    return true;
}

static inline bool getopt_test_unknown_option(void) {
    char* argv[] = {"prog", "-z", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "a"), '?', "unknown option returns '?'");
    return true;
}

static inline bool getopt_test_optopt(void) {
    char* argv[] = {"prog", "-z", NULL};
    int argc = 2;
    GETOPT_RESET();
    getopt(argc, argv, "a");
    ASSERT_EQ(optopt, 'z', "optopt set to unrecognized option character");
    return true;
}

static inline bool getopt_test_missing_required_arg(void) {
    char* argv[] = {"prog", "-o", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "o:"), '?', "missing required arg returns '?'");
    ASSERT_EQ(optopt, 'o', "optopt set to option with missing arg");
    return true;
}

static inline bool getopt_test_missing_arg_colon_prefix(void) {
    char* argv[] = {"prog", "-o", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, ":o:"), ':', "colon prefix: missing arg returns ':'");
    ASSERT_EQ(optopt, 'o', "optopt set to option with missing arg");
    return true;
}

static inline bool getopt_test_colon_prefix_unknown_still_question(void) {
    char* argv[] = {"prog", "-z", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, ":a"), '?', "colon prefix: unknown option still '?'");
    ASSERT_EQ(optopt, 'z', "optopt set to unknown option");
    return true;
}

static inline bool getopt_test_permutation(void) {
    char* argv[] = {"prog", "file1", "-a", "file2", NULL};
    int argc = 4;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "a"), 'a', "option found after permutation");
    ASSERT_EQ(getopt(argc, argv, "a"), -1, "done");
    ASSERT_TRUE(optind >= 2, "optind past all options");
    return true;
}

static inline bool getopt_test_posix_mode(void) {
    char* argv[] = {"prog", "file1", "-a", NULL};
    int argc = 3;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "+a"), -1, "'+' prefix stops at first non-option");
    ASSERT_EQ(optind, 1, "optind at first non-option");
    return true;
}

static inline bool getopt_test_in_order_mode(void) {
    char* argv[] = {"prog", "file1", "-a", NULL};
    int argc = 3;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "-a"), 1, "'-' prefix returns non-option as 1");
    ASSERT_STR_EQ(optarg, "file1", "non-option stored in optarg");
    ASSERT_EQ(getopt(argc, argv, "-a"), 'a', "option follows");
    ASSERT_EQ(getopt(argc, argv, "-a"), -1, "done");
    return true;
}

static inline bool getopt_test_optind_after_options(void) {
    char* argv[] = {"prog", "-a", "file1", "file2", NULL};
    int argc = 4;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "a"), 'a', "option parsed");
    ASSERT_EQ(getopt(argc, argv, "a"), -1, "done");
    ASSERT_EQ(optind, 2, "optind points to first non-option");
    return true;
}

static inline bool getopt_test_restart(void) {
    char* argv[] = {"prog", "-a", "-b", NULL};
    int argc = 3;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "ab"), 'a', "first parse: a");
    ASSERT_EQ(getopt(argc, argv, "ab"), 'b', "first parse: b");
    ASSERT_EQ(getopt(argc, argv, "ab"), -1, "first parse: done");

    optind = 1;
    ASSERT_EQ(getopt(argc, argv, "ab"), 'a', "restart: a");
    ASSERT_EQ(getopt(argc, argv, "ab"), 'b', "restart: b");
    ASSERT_EQ(getopt(argc, argv, "ab"), -1, "restart: done");
    return true;
}

static inline bool getopt_test_bare_dash_nonopt(void) {
    /* Bare '-' is not an option (man page: "not exactly '-' or '--'") */
    char* argv[] = {"prog", "-", "file", NULL};
    int argc = 3;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "a"), -1, "bare '-' is not an option");
    ASSERT_EQ(optind, 1, "optind at bare '-'");
    return true;
}

static inline bool getopt_test_empty_argv(void) {
    char* argv[] = {"prog", NULL};
    int argc = 1;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "abc"), -1, "empty argv returns -1");
    return true;
}

static inline bool getopt_test_repeated_after_done(void) {
    char* argv[] = {"prog", "-a", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "a"), 'a', "first call");
    ASSERT_EQ(getopt(argc, argv, "a"), -1, "exhausted");
    ASSERT_EQ(getopt(argc, argv, "a"), -1, "still -1 on repeated call");
    return true;
}

static inline bool getopt_test_combined_then_required_arg(void) {
    /* "-ab" where 'b' requires argument: rest of element after 'b' is its arg */
    char* argv[] = {"prog", "-abFILE", NULL};
    int argc = 2;
    GETOPT_RESET();
    ASSERT_EQ(getopt(argc, argv, "ab:"), 'a', "first of combined");
    ASSERT_EQ(getopt(argc, argv, "ab:"), 'b', "second of combined takes rest as arg");
    ASSERT_STR_EQ(optarg, "FILE", "rest of element is argument");
    ASSERT_EQ(getopt(argc, argv, "ab:"), -1, "done");
    return true;
}

/* ========== getopt_long tests ========== */

static inline bool getopt_long_test_no_argument(void) {
    char* argv[] = {"prog", "--verbose", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "v", opts, NULL), 'v', "--verbose returns val");
    ASSERT_EQ(getopt_long(argc, argv, "v", opts, NULL), -1, "done");
    return true;
}

static inline bool getopt_long_test_required_arg_eq(void) {
    char* argv[] = {"prog", "--output=FILE", NULL};
    int argc = 2;
    struct option opts[] = {
        {"output", required_argument, NULL, 'o'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "o:", opts, NULL), 'o', "--output=FILE");
    ASSERT_STR_EQ(optarg, "FILE", "argument via --opt=value");
    ASSERT_EQ(getopt_long(argc, argv, "o:", opts, NULL), -1, "done");
    return true;
}

static inline bool getopt_long_test_required_arg_space(void) {
    char* argv[] = {"prog", "--output", "FILE", NULL};
    int argc = 3;
    struct option opts[] = {
        {"output", required_argument, NULL, 'o'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "o:", opts, NULL), 'o', "--output FILE");
    ASSERT_STR_EQ(optarg, "FILE", "argument via --opt value");
    ASSERT_EQ(getopt_long(argc, argv, "o:", opts, NULL), -1, "done");
    return true;
}

static inline bool getopt_long_test_optional_arg_present(void) {
    char* argv[] = {"prog", "--output=FILE", NULL};
    int argc = 2;
    struct option opts[] = {
        {"output", optional_argument, NULL, 'o'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "o::", opts, NULL), 'o', "optional arg present");
    ASSERT_STR_EQ(optarg, "FILE", "optional arg via =value");
    return true;
}

static inline bool getopt_long_test_optional_arg_absent(void) {
    char* argv[] = {"prog", "--output", NULL};
    int argc = 2;
    struct option opts[] = {
        {"output", optional_argument, NULL, 'o'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "o::", opts, NULL), 'o', "optional arg absent");
    ASSERT_NULL(optarg, "optional arg absent: optarg is NULL");
    return true;
}

static inline bool getopt_long_test_optional_arg_empty_value(void) {
    /* --opt= with empty value: optarg is "" (empty string), not NULL */
    char* argv[] = {"prog", "--output=", NULL};
    int argc = 2;
    struct option opts[] = {
        {"output", optional_argument, NULL, 'o'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "o::", opts, NULL), 'o', "optional arg empty =");
    ASSERT_NOT_NULL(optarg, "optarg is not NULL for --opt=");
    ASSERT_STR_EQ(optarg, "", "optarg is empty string for --opt=");
    return true;
}

static inline bool getopt_long_test_flag_pointer(void) {
    char* argv[] = {"prog", "--verbose", NULL};
    int argc = 2;
    int flag_val = 0;
    struct option opts[] = {
        {"verbose", no_argument, &flag_val, 42},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "", opts, NULL), 0, "flag != NULL: returns 0");
    ASSERT_EQ(flag_val, 42, "flag pointer: *flag set to val");
    return true;
}

static inline bool getopt_long_test_flag_not_set_when_absent(void) {
    char* argv[] = {"prog", NULL};
    int argc = 1;
    int flag_val = 99;
    struct option opts[] = {
        {"verbose", no_argument, &flag_val, 42},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "", opts, NULL), -1, "no options");
    ASSERT_EQ(flag_val, 99, "flag left unchanged when option not present");
    return true;
}

static inline bool getopt_long_test_val_returned(void) {
    char* argv[] = {"prog", "--verbose", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 1001},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "", opts, NULL), 1001, "null flag: val is returned");
    return true;
}

static inline bool getopt_long_test_longindex(void) {
    char* argv[] = {"prog", "--beta", "--alpha", NULL};
    int argc = 3;
    int longindex = -1;
    struct option opts[] = {
        {"alpha", no_argument, NULL, 'a'},
        {"beta",  no_argument, NULL, 'b'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    getopt_long(argc, argv, "ab", opts, &longindex);
    ASSERT_EQ(longindex, 1, "longindex for --beta is 1");
    getopt_long(argc, argv, "ab", opts, &longindex);
    ASSERT_EQ(longindex, 0, "longindex for --alpha is 0");
    return true;
}

static inline bool getopt_long_test_mixed_short_long(void) {
    char* argv[] = {"prog", "-a", "--beta", "-c", NULL};
    int argc = 4;
    struct option opts[] = {
        {"beta", no_argument, NULL, 'b'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "abc", opts, NULL), 'a', "short -a");
    ASSERT_EQ(getopt_long(argc, argv, "abc", opts, NULL), 'b', "long --beta");
    ASSERT_EQ(getopt_long(argc, argv, "abc", opts, NULL), 'c', "short -c");
    ASSERT_EQ(getopt_long(argc, argv, "abc", opts, NULL), -1, "done");
    return true;
}

static inline bool getopt_long_test_abbreviation(void) {
    char* argv[] = {"prog", "--verb", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "v", opts, NULL), 'v', "unique prefix --verb matches --verbose");
    return true;
}

static inline bool getopt_long_test_exact_match_not_ambiguous(void) {
    /* --verbose is an exact match even though --verbose-mode also exists */
    char* argv[] = {"prog", "--verbose", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose",      no_argument, NULL, 'v'},
        {"verbose-mode", no_argument, NULL, 'm'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "vm", opts, NULL), 'v', "exact match wins over ambiguous prefix");
    return true;
}

static inline bool getopt_long_test_ambiguous(void) {
    char* argv[] = {"prog", "--ver", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {"version", no_argument, NULL, 'V'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "vV", opts, NULL), '?', "ambiguous --ver returns '?'");
    return true;
}

static inline bool getopt_long_test_unknown_long(void) {
    char* argv[] = {"prog", "--unknown", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "v", opts, NULL), '?', "unknown long option returns '?'");
    return true;
}

static inline bool getopt_long_test_extraneous_arg(void) {
    /* no_argument option given --opt=value: extraneous param returns '?' */
    char* argv[] = {"prog", "--verbose=yes", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "v", opts, NULL), '?', "extraneous arg to no_argument returns '?'");
    return true;
}

static inline bool getopt_long_test_double_dash_terminates(void) {
    char* argv[] = {"prog", "--verbose", "--", "--output", NULL};
    int argc = 4;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {"output",  no_argument, NULL, 'o'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "vo", opts, NULL), 'v', "--verbose parsed");
    ASSERT_EQ(getopt_long(argc, argv, "vo", opts, NULL), -1, "-- terminates processing");
    ASSERT_EQ(optind, 3, "optind points past --");
    return true;
}

static inline bool getopt_long_test_empty_optstring(void) {
    char* argv[] = {"prog", "--verbose", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "", opts, NULL), 'v', "empty optstring: long opts work");
    return true;
}

static inline bool getopt_long_test_permutation(void) {
    char* argv[] = {"prog", "file", "--verbose", NULL};
    int argc = 3;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long(argc, argv, "v", opts, NULL), 'v', "long opt found after permutation");
    ASSERT_EQ(getopt_long(argc, argv, "v", opts, NULL), -1, "done");
    return true;
}

/* ========== getopt_long_only tests ========== */

static inline bool getopt_long_only_test_single_dash_long(void) {
    char* argv[] = {"prog", "-verbose", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long_only(argc, argv, "", opts, NULL), 'v', "single dash long opt recognized");
    ASSERT_EQ(getopt_long_only(argc, argv, "", opts, NULL), -1, "done");
    return true;
}

static inline bool getopt_long_only_test_double_dash_also_works(void) {
    char* argv[] = {"prog", "--verbose", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long_only(argc, argv, "", opts, NULL), 'v', "double dash still works");
    return true;
}

static inline bool getopt_long_only_test_fallback_to_short(void) {
    /* -a has no long match, but 'a' is a valid short option */
    char* argv[] = {"prog", "-a", NULL};
    int argc = 2;
    struct option opts[] = {
        {"all", no_argument, NULL, 1000},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    /* "-a" doesn't match "--all", but 'a' is in optstring */
    ASSERT_EQ(getopt_long_only(argc, argv, "a", opts, NULL), 'a', "fallback to short when no long match");
    return true;
}

static inline bool getopt_long_only_test_prefers_long_over_short(void) {
    /* -verbose should match long --verbose (val=1000), not short -v */
    char* argv[] = {"prog", "-verbose", NULL};
    int argc = 2;
    struct option opts[] = {
        {"verbose", no_argument, NULL, 1000},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long_only(argc, argv, "v", opts, NULL), 1000, "long match preferred over short 'v'");
    ASSERT_EQ(getopt_long_only(argc, argv, "v", opts, NULL), -1, "done after long opt consumed");
    return true;
}

static inline bool getopt_long_only_test_single_dash_with_arg(void) {
    char* argv[] = {"prog", "-output=FILE", NULL};
    int argc = 2;
    struct option opts[] = {
        {"output", required_argument, NULL, 'o'},
        {NULL, 0, NULL, 0}
    };
    GETOPT_RESET();
    ASSERT_EQ(getopt_long_only(argc, argv, "o:", opts, NULL), 'o', "single dash long opt with =arg");
    ASSERT_STR_EQ(optarg, "FILE", "argument via = with single dash");
    return true;
}

/* ========== runner ========== */

static inline bool getopt_run_all_tests(void) {
    int passed = 0;
    int total = 44;

    /* getopt tests */
    if (getopt_test_basic_option()) passed++;
    if (getopt_test_multiple_options()) passed++;
    if (getopt_test_combined_options()) passed++;
    if (getopt_test_required_arg_inline()) passed++;
    if (getopt_test_required_arg_next()) passed++;
    if (getopt_test_optional_arg_present()) passed++;
    if (getopt_test_optional_arg_absent()) passed++;
    if (getopt_test_optional_arg_not_next_element()) passed++;
    if (getopt_test_no_options()) passed++;
    if (getopt_test_double_dash()) passed++;
    if (getopt_test_unknown_option()) passed++;
    if (getopt_test_optopt()) passed++;
    if (getopt_test_missing_required_arg()) passed++;
    if (getopt_test_missing_arg_colon_prefix()) passed++;
    if (getopt_test_colon_prefix_unknown_still_question()) passed++;
    if (getopt_test_permutation()) passed++;
    if (getopt_test_posix_mode()) passed++;
    if (getopt_test_in_order_mode()) passed++;
    if (getopt_test_optind_after_options()) passed++;
    if (getopt_test_restart()) passed++;
    if (getopt_test_bare_dash_nonopt()) passed++;
    if (getopt_test_empty_argv()) passed++;
    if (getopt_test_repeated_after_done()) passed++;
    if (getopt_test_combined_then_required_arg()) passed++;

    /* getopt_long tests */
    if (getopt_long_test_no_argument()) passed++;
    if (getopt_long_test_required_arg_eq()) passed++;
    if (getopt_long_test_required_arg_space()) passed++;
    if (getopt_long_test_optional_arg_present()) passed++;
    if (getopt_long_test_optional_arg_absent()) passed++;
    if (getopt_long_test_optional_arg_empty_value()) passed++;
    if (getopt_long_test_flag_pointer()) passed++;
    if (getopt_long_test_flag_not_set_when_absent()) passed++;
    if (getopt_long_test_val_returned()) passed++;
    if (getopt_long_test_longindex()) passed++;
    if (getopt_long_test_mixed_short_long()) passed++;
    if (getopt_long_test_abbreviation()) passed++;
    if (getopt_long_test_exact_match_not_ambiguous()) passed++;
    if (getopt_long_test_ambiguous()) passed++;
    if (getopt_long_test_unknown_long()) passed++;
    if (getopt_long_test_extraneous_arg()) passed++;
    if (getopt_long_test_double_dash_terminates()) passed++;
    if (getopt_long_test_empty_optstring()) passed++;
    if (getopt_long_test_permutation()) passed++;

    /* getopt_long_only tests */
    if (getopt_long_only_test_single_dash_long()) passed++;
    if (getopt_long_only_test_double_dash_also_works()) passed++;
    if (getopt_long_only_test_fallback_to_short()) passed++;
    if (getopt_long_only_test_prefers_long_over_short()) passed++;
    if (getopt_long_only_test_single_dash_with_arg()) passed++;

    if (passed == total) {
        debug_printf("getopt: %d/%d PASSED\n", passed, total);
        return true;
    } else {
        debug_printf("getopt: %d/%d FAILED\n", passed, total);
        return false;
    }
}

#endif
