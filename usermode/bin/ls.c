#include <sys/stat.h>
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

#define OPT_SHORT "1Ghl"

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
    { "oneline", no_argument, NULL, '1' },
    { "long", no_argument, NULL, 'l' },
    { "grid", no_argument, NULL, 'G' },
    { "color", optional_argument, NULL, 0 },
    { "human-readable", no_argument, NULL, 'h' },
    { "si", no_argument, NULL, 0 },
    { NULL, 0, NULL, 0 },
};

static void print_usage(void)
{
    puts("Usage:");
    puts("  ls [options] [paths...]");
    puts("");
    puts("META OPTIONS");
    puts("  --help                  Shows usage info");
    puts("");
    puts("DISPLAY OPTIONS");
    puts("  -1, --oneline           Display one entry per line");
    puts("  -l, --long              Display extended metadata as a table");
    puts("  -G, --grid              Display entries as a grid (default mode)");
    puts("  --color[=WHEN]          Enables/disables coloring (more below)");
    puts("");
    puts("TABLE OPTIONS");
    puts("  -h, --human-readable    Print sizes like '1k', '234M', '2G' etc.");
    puts("  --si                    Like -h, but use powers of 1000 instead of 1024");
    puts("");
    puts("ARGUMENTS");
    puts("  [paths...]              One or more directory paths to print. Defaults to current working directory.");
    puts("");
    puts("DETAILS");
    puts("  --color[=WHEN] enables or disables coloring. By default, coloring is disabled. \"WHEN\" can equal \"auto\", \"always\", or \"never\".");
    puts("  If --color is passed, but without value, it is treated as \"always. If set to \"auto\", it will only apply coloring when output is");
    puts("  connected to TTY.");
}

static char* months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

static char* units[] = {
    "", "k", "M", "G", "T", "P", "E", "Z", "Y", "R", "Q"
};

// One for each value in ls_entry_type_t, in order
static char* typechars = "bcdfl.s?";

// Color escape codes, one for each value in ls_entry_type_t, in order.
static char* color_defs[] = {
    "\033[1;93m",
    "\033[1;93m",
    "\033[1;96m",
    "\033[95m",
    "\033[92m",
    "\033[97m",
    "\033[90m",
    "\033[91m",
};

typedef enum {
    DM_GRID,
    DM_ONE_LINE,
    DM_TABULAR,
} display_mode_t;

typedef enum {
    SU_BYTES,
    SU_KIBI,
    SU_SI,
} size_unit_t;

static display_mode_t display_mode = DM_GRID;
static size_unit_t size_unit = SU_BYTES;
static int terminal_width = 64; // TODO: read real width from terminal
static int curr_time;
static bool enable_colors = false;

typedef enum {
    ET_BLK,
    ET_CHR,
    ET_DIR,
    ET_FIFO,
    ET_LNK,
    ET_REG,
    ET_SOCK,
    ET_UNKNOWN,
} ls_entry_type_t;

typedef struct {
    char name[PATH_MAX_LENGTH];
    ls_entry_type_t type;
    bool stat_loaded;
    off_t size;
    struct timespec last_mod_time;
    char link_path[PATH_MAX_LENGTH];
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
        printf(
            "%s%s%s\n",
            enable_colors ? color_defs[entry->type] : "",
            entry->name,
            enable_colors ? "\033[0m" : ""
        );
        return;
    }

    if (display_mode == DM_GRID) {
        int col_width = longest_name_len + 2;
        int columns = terminal_width / col_width;
        char format[32] = {0};
        sprintf(format, "%%s%%-%us%%s", col_width);
        printf(
            format,
            enable_colors ? color_defs[entry->type] : "",
            entry->name,
            enable_colors ? "\033[0m" : ""
        );
        if (i % columns == (columns - 1)) printf("\n");
        return;
    }

    // Build type & permissions string
    char type_and_perm[32] = {0};
    sprintf(
        type_and_perm,
        "%s%c%s---------%s",
        enable_colors ? color_defs[entry->type] : "",
        typechars[entry->type],
        enable_colors ? "\033[97m" : "",
        enable_colors ? "\033[0m" : ""
    );

    // Build size string
    char size_string[64] = {0};
    if (size_unit == SU_BYTES) {
        // Calculate number of characters needed for size
        char maxlenbuf[64];
        sprintf(maxlenbuf, "%u", largest_size);
        int chars_for_size = strlen(maxlenbuf);

        char size_string_format[32] = {0};
        sprintf(size_string_format, "%%s%%%uu%%s", chars_for_size);
        sprintf(
            size_string,
            size_string_format,
            enable_colors ? "\033[93m" : "",
            entry->size,
            enable_colors ? "\033[0m" : ""
        );
    } else {
        off_t power = size_unit == SU_SI ? 1000 : 1024;
        int unit_level = 0;
        off_t curr_size = entry->size;
        while (curr_size > power) {
            unit_level++;
            curr_size /= power;
        }
        sprintf(
            size_string,
            unit_level == 0 ? "%s%5u%s" : "%s%4u%s%s",
            enable_colors ? (unit_level == 0 ? "\033[93m" : "\033[1;93m") : "",
            curr_size,
            units[unit_level],
            enable_colors ? "\033[0m" : ""
        );
    }

    // Build last modification string
    struct tm* time_tm = localtime(&entry->last_mod_time.tv_spec);
    char mod_time[32] = {0};
    sprintf(
        mod_time,
        "%s%s %2u %02u:%02u%s",
        enable_colors ? "\033[95m" : "",
        months[time_tm->tm_mon],
        time_tm->tm_mday,
        time_tm->tm_hour,
        time_tm->tm_min,
        enable_colors ? "\033[0m" : ""
    );

    // Build name, including link if necessary
    char entry_name[PATH_MAX_LENGTH * 2];
    if (entry->type == ET_LNK && entry->link_path[0]) {
        sprintf(
            entry_name,
            "%s%s%s -> %s",
            enable_colors ? color_defs[entry->type] : "",
            entry->name,
            enable_colors ? "\033[0m" : "",
            entry->link_path

        );
    } else {
        sprintf(
            entry_name,
            "%s%s%s",
            enable_colors ? color_defs[entry->type] : "",
            entry->name,
            enable_colors ? "\033[0m" : ""
        );
    }

    printf(
        "%s %s %s %s\n",
        type_and_perm,
        size_string,
        mod_time,
        entry_name
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

        switch (dirent->d_type) {
            case DT_BLK: entries[entries_count].type = ET_BLK; break;
            case DT_CHR: entries[entries_count].type = ET_CHR; break;
            case DT_DIR: entries[entries_count].type = ET_DIR; break;
            case DT_FIFO: entries[entries_count].type = ET_FIFO; break;
            case DT_LNK: entries[entries_count].type = ET_LNK; break;
            case DT_REG: entries[entries_count].type = ET_REG; break;
            case DT_SOCK: entries[entries_count].type = ET_SOCK; break;
            default: entries[entries_count].type = ET_UNKNOWN; break;
        }

        if (display_mode == DM_TABULAR) {
            char full_path[PATH_MAX_LENGTH] = {0};
            sprintf(
                full_path,
                "%s/%s",
                path,
                dirent->d_name
            );

            if (lstat(full_path, &statbuf) < 0) {
                entries[entries_count].stat_loaded = false;
                entries[entries_count].size = 0;
                entries[entries_count].last_mod_time.tv_nsec = 0;
                entries[entries_count].last_mod_time.tv_spec = curr_time;
                result = 1;
            } else {
                entries[entries_count].stat_loaded = true;
                entries[entries_count].size = statbuf.st_size;
                entries[entries_count].last_mod_time = statbuf.st_mtim;
                if (statbuf.st_size > largest_size) {
                    largest_size = statbuf.st_size;
                }
            }

            if (dirent->d_type == DT_LNK) {
                int res = readlink(
                    full_path,
                    entries[entries_count].link_path,
                    PATH_MAX_LENGTH - 1
                );
                if (res < 0) {
                    memset(entries[entries_count].link_path, 0, PATH_MAX_LENGTH);
                    result = 1;
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
                else if (!strcmp(opts[option_index].name, "si")) {
                    size_unit = SU_SI;
                }
                else if (!strcmp(opts[option_index].name, "color")) {
                    if (optarg == 0) {
                        enable_colors = true;
                    } else if (!strcmp(optarg, "never")) {
                        enable_colors = false;
                    } else if (!strcmp(optarg, "always")) {
                        enable_colors = true;
                    } else if (!strcmp(optarg, "auto")) {
                        enable_colors = isatty(STDOUT_FILENO);
                    } else {
                        dprintf(
                            STDERR_FILENO,
                            "ls: option --color has no \"%s\" setting\n",
                            optarg
                        );
                        return 1;
                    }
                }
                break;
            }
            case '1': display_mode = DM_ONE_LINE; break;
            case 'G': display_mode = DM_GRID; break;
            case 'h': size_unit = SU_KIBI; break;
            case 'l': display_mode = DM_TABULAR; break;
            default: break;
        }
    }

    curr_time = time(NULL);
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
