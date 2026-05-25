#include "unistd.h"
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define PATH_MAX_LENGTH 256

#define OPT_SHORT "1lG"

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
    { "oneline", no_argument, NULL, '1' },
    { "long", no_argument, NULL, 'l' },
    { "grid", no_argument, NULL, 'G' },
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
    puts("  -1, --oneline       Display one entry per line");
    puts("  -l, --long          Display extended metadata as a table");
    puts("  -G, --grid          Display entries as a grid (default mode)");
    puts("");
    puts("ARGUMENTS");
    puts("  [paths...]          One or more directory paths to print.");
    puts("                      Defaults to current working directory.");
}

typedef enum {
    DM_GRID,
    DM_ONE_LINE,
    DM_TABULAR,
} display_mode_t;

static display_mode_t display_mode = DM_GRID;

static void list_for_path(const char* path)
{
    printf(">>CONTENT OF PATH %s<<\n", path);
}

int main(int argc, char** argv)
{
    bool help = false;
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
            case '1': display_mode = DM_ONE_LINE; break;
            case 'l': display_mode = DM_TABULAR; break;
            case 'G': display_mode = DM_GRID; break;
            default: break;
        }
    }

    if (help) {
        print_usage();
        return 0;
    }

    if (optind < argc) {
        int paths_count = argc - optind;
        bool print_paths = paths_count > 1;

        for (int i = 0; i < paths_count; i++) {
            char* path = argv[optind + i];
            if (print_paths) {
                printf("%s:\n", path);
            }
            list_for_path(path);
            if (print_paths && i < (paths_count - 1)) {
                puts("");
            }
        }
    } else {
        char path[PATH_MAX_LENGTH] = {0};
        getcwd(path, PATH_MAX_LENGTH - 1);
        list_for_path(path);
    }

    return 0;
}
