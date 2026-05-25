#include "unistd.h"
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define OPT_SHORT "l"

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
    { "long", no_argument, NULL, 'l' },
    { NULL, 0, NULL, 0 },
};

static void print_usage(void)
{
    puts("Uage:");
    puts("  ls [options] [paths...]");
    puts("");
    puts("META OPTIONS");
    puts("  --help              Shows usage info");
    puts("");
    puts("DISPLAY OPTIONS");
    puts("  -l, --long          Display extended metadata as a table");
    puts("");
    puts("ARGUMENTS");
    puts("  [paths...]          One or more directory paths to print.");
    puts("                      Defaults to current working directory.");
}

int main(int argc, char** argv)
{
    bool help = false;
    bool tabular = false;
    int c;
    while (true) {
        int option_index = 0;
        c = getopt_long(argc, argv, OPT_SHORT, opts, &option_index);

        if (c == -1) break;
        switch (c) {
            case 0: {
                if (!strcmp(opts[option_index].name, "help")) {
                    help = true;
                }
                break;
            }
            case 'l': tabular = true; break;
            default: break;
        }
    }

    if (help) {
        print_usage();
        return 0;
    }

    if (optind < argc) {
        printf("Non-option argv elements:");
        while (optind < argc) {
            printf("\n- '%s'", argv[optind++]);
        }
        printf("\n");
    }

    printf(
        "help=%s, tabular=%s\n",
        help ? "true" : "false",
        tabular ? "true": "false"
    );
    return 0;
}
