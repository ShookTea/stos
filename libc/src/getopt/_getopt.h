#ifndef _LIBC_SRC_GETOPT_H
#define _LIBC_SRC_GETOPT_H

#include <getopt.h>
#include <stdbool.h>

typedef enum {
    PERMUTE,
    RETURN_IN_ORDER,
    REQUIRE_ORDER
}  ordering_t;

int __getopt_internal(
    int argc,
    char *argv[],
    const char *optstring,
    const struct option *longopts,
    int *longindex,
    bool long_only
);

#endif
