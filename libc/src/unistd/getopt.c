#include <unistd.h>

char* optarg = NULL;
int optind = 1, opterr = 1, optopt = 0;

int getopt(
    int argc __attribute__((unused)),
    char *argv[] __attribute__((unused)),
    const char *optstring __attribute__((unused))
) {
    return 0;
}
