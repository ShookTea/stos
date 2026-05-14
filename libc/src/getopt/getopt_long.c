#include <getopt.h>
#include <stdbool.h>
#include "./_getopt.h"

int getopt_long(
    int argc,
    char *argv[],
    const char *optstring,
    const struct option *longopts,
    int *longindex
) {
    return __getopt_internal(
        argc,
        argv,
        optstring,
        longopts,
        longindex,
        false
    );
}
