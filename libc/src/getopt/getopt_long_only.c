#include <getopt.h>

int getopt_long_only(
    int argc __attribute__((unused)),
    char *argv[] __attribute__((unused)),
    const char *optstring __attribute__((unused)),
    const struct option *longopts __attribute__((unused)),
    int *longindex __attribute__((unused))
) {
    return 0;
}
