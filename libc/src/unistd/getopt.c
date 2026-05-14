#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "../getopt/_getopt.h"

int getopt(int argc, char *argv[], const char *optstring) {
    return __getopt_internal(
        argc,
        argv,
        optstring,
        NULL,
        NULL,
        false
    );
}
