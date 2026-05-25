#include "sys/stat.h"
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
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
static int terminal_width = 64; // TODO: read real width from terminal

typedef struct {
    char name[PATH_MAX_LENGTH];
    char type;
    bool stat_loaded;
    off_t size;
    struct timespec last_mod_time;
} ls_entry_t;

static int sort_comp(const void* _a, const void* _b)
{
    ls_entry_t* a = (ls_entry_t*)_a;
    ls_entry_t* b = (ls_entry_t*)_b;
    return strcmp(a->name, b->name);
}

static void print_entry(
    int i,
    ls_entry_t* entry,
    int longest_name_len,
    off_t largest_size
) {
    if (display_mode == DM_ONE_LINE) {
        printf("%s\n", entry->name);
        return;
    }

    if (display_mode == DM_GRID) {
        int col_width = longest_name_len + 2;
        int columns = terminal_width / col_width;
        char format[32] = {0};
        sprintf(format, "%%-%us", col_width);
        printf(format, entry->name);
        if (i % columns == (columns - 1)) printf("\n");
        return;
    }

    // Calculate number of characters needed for size
    char maxlenbuf[64];
    sprintf(maxlenbuf, "%u", largest_size);
    int chars_for_size = strlen(maxlenbuf);

    // Build format string
    char format[32] = {0};
    sprintf(
        format,
        "%%c--------- %%-%uu %%s %%s\n",
        chars_for_size
    );

    // Load stats
    off_t size;
    struct tm* time_tm;
    if (entry->stat_loaded) {
        size = entry->size;
        time_tm = localtime(&entry->last_mod_time.tv_spec);
    } else {
        size = 0;
        time_t t = time(NULL);
        time_tm = localtime(&t);
    }

    // Build last modification string
    char mod_time[13] = {0};
    sprintf(
        mod_time,
        "%s %2u %02u:%02u",
        "May", // TODO: load real month
        time_tm->tm_mday,
        time_tm->tm_hour,
        time_tm->tm_min
    );

    printf(
        format,
        entry->type,
        size,
        mod_time,
        entry->name
    );
    return;
}

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

    ls_entry_t* entries = NULL;
    int entries_count = 0;
    int longest_name_len = 0;
    off_t largest_size = 0;

    struct dirent* dirent;
    struct stat statbuf;
    int result = 0;
    while ((dirent = readdir(dir)) != NULL) {
        int len = strlen(dirent->d_name);
        entries = realloc(entries, sizeof(ls_entry_t) * (entries_count + 1));
        strcpy(entries[entries_count].name, dirent->d_name);

        if (display_mode == DM_TABULAR) {
            switch (dirent->d_type) {
                case DT_BLK: entries[entries_count].type = 'b'; break;
                case DT_CHR: entries[entries_count].type = 'c'; break;
                case DT_DIR: entries[entries_count].type = 'd'; break;
                case DT_FIFO: entries[entries_count].type = 'f'; break;
                case DT_LNK: entries[entries_count].type = 'l'; break;
                case DT_REG: entries[entries_count].type = '.'; break;
                case DT_SOCK: entries[entries_count].type = 's'; break;
                default: entries[entries_count].type = '?'; break;
            }

            char full_path[PATH_MAX_LENGTH] = {0};
            sprintf(
                full_path,
                "%s/%s",
                path,
                dirent->d_name
            );

            if (lstat(full_path, &statbuf) < 0) {
                entries[entries_count].stat_loaded = false;
                result = 1;
            } else {
                entries[entries_count].stat_loaded = true;
                entries[entries_count].size = statbuf.st_size;
                entries[entries_count].last_mod_time = statbuf.st_mtim;
                if (statbuf.st_size > largest_size) {
                    largest_size = statbuf.st_size;
                }
            }

        }

        if (len > longest_name_len) {
            longest_name_len = len;
        }

        entries_count++;
    }
    closedir(dir);

    qsort(entries, entries_count, sizeof(ls_entry_t), sort_comp);

    // Printing and cleanup
    for (int i = 0; i < entries_count; i++) {
        print_entry(i, entries + i, longest_name_len, largest_size);
    }
    free(entries);
    return result;
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
