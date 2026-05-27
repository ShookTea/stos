#include "fcntl.h"
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#define LINE_BUF 4096

#define OPT_SHORT ""

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
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

static void run_grep(
    const char* pattern,
    int fd,
    const char* filename __attribute__((unused))
) {
    char line[LINE_BUF];
    int len = 0;
    char c;
    while (read(fd, &c, 1) == 1) {
        if (len < LINE_BUF - 1) {
            line[len++] = c;
        }
        if (c == '\n' || len == LINE_BUF - 1) {
            line[len] = '\0';
            if (strstr(line, pattern) != NULL) {
                write(1, line, len);
            }
            len = 0;
        }
    }

    if (len > 0) {
        line[len] = '\0';
        if (strstr(line, pattern) != NULL) {
            write(1, line, len);
        }
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
            }
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
