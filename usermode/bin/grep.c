#include "fcntl.h"
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#define LINE_BUF 4096

#define OPT_SHORT "vclL"

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
    { "invert-match", no_argument, NULL, 'v' },
    { "count", no_argument, NULL, 'c' },
    { "files-with-matches", no_argument, NULL, 'l' },
    { "files-without-match", no_argument, NULL, 'L' },
    { NULL, 0, NULL, 0 },
};

static void print_usage(void)
{
    puts("Usage:");
    puts("  grep [options] <pattern> [files...]");
    puts("");
    puts("META OPTIONS");
    puts("  --help                  Shows usage info");
    puts("");
    puts("MATCHING OPTIONS");
    puts("  -v, --invert-match      Invert matching, printing only non-matched lines");
    puts("");
    puts("GENERAL OUTPUT OPTIONS");
    puts("  -c, --count             Instead of matched lines, print count of matched lines");
    puts("                          for each file.");
    puts("  -l,");
    puts("  --files-with-matches    Instead of matched lines, print names of files with");
    puts("                          matched line. End scanning after first match.");
    puts("  -L,");
    puts("  --files-without-match   Instead of matched lines, print names of files with");
    puts("                          no matched line.");
    puts("");
    puts("ARGUMENTS");
    puts("  <pattern>               Pattern used for searching in the input.");
    puts("  [files...]              One or more files to search for the pattern.");
    puts("                          \"-\" stands for standard input. If no files are");
    puts("                          given, it will default to standard input.");
}

static void print_error(const char* error)
{
    dprintf(
        STDERR_FILENO,
        "%s.\nRun \"grep --help\" to get usage help.\n",
        error
    );
}

static bool invert_match = false;
static bool print_count_only = false;
static bool print_files_with_match_only = false;
static bool print_files_without_match_only = false;

static void run_grep(
    const char* pattern,
    int fd,
    const char* filename
) {
    char line[LINE_BUF];
    int len = 0;
    int matched_count = 0;
    char c;
    bool print_matched = !print_count_only
        && !print_files_with_match_only
        && !print_files_without_match_only;

    while (read(fd, &c, 1) == 1) {
        if (len < LINE_BUF - 1) {
            line[len++] = c;
        }
        if (c == '\n' || len == LINE_BUF - 1) {
            line[len] = '\0';
            bool found = strstr(line, pattern) != NULL;
            if (found != invert_match) {
                matched_count++;
                if (print_files_without_match_only
                    || print_files_with_match_only) {
                        break;
                    }
                if (print_matched) write(1, line, len);
            }
            len = 0;
        }
    }

    if (len > 0) {
        line[len] = '\0';
        bool found = strstr(line, pattern) != NULL;
        if (found != invert_match) {
            matched_count++;
            if (print_matched) write(1, line, len);
        }
    }

    if (print_count_only) {
        printf("%u\n", matched_count);
    }
    else if (print_files_with_match_only) {
        if (matched_count > 0) printf("%s\n", filename);
    }
    else if (print_files_without_match_only) {
        if (matched_count == 0) printf("%s\n", filename);
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        print_error("Missing pattern");
        return 1;
    }

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
            case 'v': invert_match = true; break;
            case 'c': print_count_only = true; break;
            case 'l': print_files_with_match_only = true; break;
            case 'L': print_files_without_match_only = true; break;
            default: break;
        }
    }

    if (optind >= argc) {
        print_error("Missing pattern");
        return 1;
    }

    char* pattern = argv[optind];
    if (optind + 1 < argc) {
        int files_count = argc - (optind + 1);
        for (int i = 0; i < files_count; i++) {
            char* path = argv[optind + i + 1];
            if (!strcmp(path, "-")) {
                // placeholder for standard input
                run_grep(pattern, 0, "(standard input)");
            } else {
                int fd = open(path, O_RDONLY);
                if (fd < 0) {
                    dprintf(STDERR_FILENO, "Path '%s' is invalid.\n", path);
                    continue;
                }
                run_grep(pattern, fd, path);
                close(fd);
            }
        }
    } else {
        // Only standard input used
        run_grep(pattern, 0, "(standard input)");
    }

    return 0;
}
