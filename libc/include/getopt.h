#ifndef _SYS_GETOPT_H
#define _SYS_GETOPT_H 1
#include <sys/cdefs.h>

/**
 * TODO: write documentation (https://man7.org/linux/man-pages/man3/getopt.3.html)
 */
struct option {
    const char* name;
    int has_arg;
    int* flag;
    int val;
};

#define no_argument 0
#define required_argument 1
#define optional_argument 2

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parses command line arguments, including long form.
 * TODO: write documentation (https://man7.org/linux/man-pages/man3/getopt.3.html)
 */
int getopt_long(
    int argc,
    char *argv[],
    const char *optstring,
    const struct option *longopts,
    int *longindex
);

/**
 * Parses command line arguments, using long form only.
 * TODO: write documentation (https://man7.org/linux/man-pages/man3/getopt.3.html)
 */
int getopt_long_only(
    int argc,
    char *argv[],
    const char *optstring,
    const struct option *longopts,
    int *longindex
);

#ifdef __cplusplus
}
#endif

#endif
