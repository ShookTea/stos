#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// TODO: add support for -p / --parent flag
#define OPT_SHORT ""

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
    { NULL, 0, NULL, 0 },
};

static void print_usage(void)
{
    puts("Usage:");
    puts("  mkdir [options] <directories...>");
    puts("");
    puts("META OPTIONS");
    puts("  --help                  Shows usage info");
    puts("");
    puts("ARGUMENTS");
    puts("  <directories...>        Paths to at least one directory name to be created.");
}

int main(int argc, char** argv)
{
    int c;
    while (true) {
        int option_index = 0;
        c = getopt_long(argc, argv, OPT_SHORT, opts, &option_index);

        if (c == -1) break;
        switch (c) {
            case 0: {
                if (!strcmp(opts[option_index].name, "help")) {
                    print_usage();
                    return 0;
                }
                break;
            }
            default: break;
        }
    }

    if (optind >= argc) {
        dprintf(STDERR_FILENO, "mkdir: at least one path is required.\n");
        return 1;
    }

    int paths_count = argc - optind;
    int result = 0;
    for (int i = 0; i < paths_count; i++) {
        char* path = argv[optind + i];
        int res = mkdir(path, 0644);
        if (res != 0) {
            result = 2;
            dprintf(STDERR_FILENO, "error when creating dir '%s': ", path);
            switch (errno) {
                case EBADF:
                    dprintf(STDERR_FILENO, "parent doesn't exist.\n");
                    break;
                case ENOENT:
                    dprintf(STDERR_FILENO, "parent doesn't exist.\n");
                    break;
                case ENOTDIR:
                    dprintf(STDERR_FILENO, "parent is not a directory.\n");
                    break;
                case EEXIST:
                    dprintf(STDERR_FILENO, "path already exists.\n");
                    break;
                default:
                    dprintf(STDERR_FILENO, "unknown error.\n");
            }
        }
    }

    return result;
}
