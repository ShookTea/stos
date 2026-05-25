#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
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

static int list_for_path(const char* path)
{
    errno = 0;
    DIR* dir = opendir(path);
    if (errno != 0) {
        printf("Cannot read content of '%s': ", path);
        switch (errno) {
            case EACCES: puts("permission denied"); break;
            case ENOENT: puts("directory doesn't exist"); break;
            case ENOTDIR: puts("path is not a directory"); break;
            default: puts("Unrecognized error");
        }
        return 1;
    }

    char** names_list = NULL;
    int names_count = 0;
    int longest_name_len = 0;

    struct dirent* dirent;
    while ((dirent = readdir(dir)) != NULL) {
        int len = strlen(dirent->d_name);
        names_list = realloc(names_list, sizeof(char*) * (names_count + 1));
        names_list[names_count] = malloc(sizeof(char) * (len + 1));
        strcpy(names_list[names_count], dirent->d_name);
        names_count++;
        if (len > longest_name_len) {
            longest_name_len = len;
        }
    }

    closedir(dir);

    // Cleanup
    for (int i = 0; i < names_count; i++) {
        free(names_list[i]);
    }
    free(names_list);
    return 0;
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

    int result = 0;

    if (optind < argc) {
        int paths_count = argc - optind;
        bool print_paths = paths_count > 1;

        for (int i = 0; i < paths_count; i++) {
            char* path = argv[optind + i];
            if (print_paths) {
                printf("%s:\n", path);
            }

            int res = list_for_path(path);
            if (res > result) {
                result = res;
            }

            if (print_paths && i < (paths_count - 1)) {
                puts("");
            }
        }
    } else {
        char path[PATH_MAX_LENGTH] = {0};
        getcwd(path, PATH_MAX_LENGTH - 1);
        result = list_for_path(path);
    }

    return result;
}
