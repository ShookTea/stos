#include "unistd.h"
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
    { "long", no_argument, NULL, 'l' },
    { NULL, 0, NULL, 0 },
};

int main(int argc, char** argv)
{
    bool help = false;
    bool tabular = false;
    int c;
    while (true) {
        int option_index = 0;
        c = getopt_long(argc, argv, "l", opts, &option_index);

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
