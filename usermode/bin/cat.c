#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define OPT_SHORT "En"

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
    { "show-ends", no_argument, NULL, 'E' },
    { "number", no_argument, NULL, 'n' },
    { NULL, 0, NULL, 0 },
};

static void print_usage(void)
{
    puts("Usage:");
    puts("  cat [files...]");
    puts("");
    puts("META OPTIONS");
    puts("  --help              Shows usage info");
    puts("");
    puts("OUTPUT OPTIONS");
    puts("  -E, --show-ends     Display '$' at the end of each line");
    puts("  -n, --number        Number all output lines");
    puts("");
    puts("ARGUMENTS:");
    puts("  [files...]          Concatenated files. \"-\" stands for standard input.");
    puts("                      If no files are given, it will default to standard input.");
}

#define LINE_BUF 4096

static bool opt_show_ends = false;
static bool opt_number = false;
static int line_number = 0;

static void print_lines(FILE* file)
{
    char line[LINE_BUF];
    while (fgets(line, LINE_BUF, file)) {
        int len = strlen(line);
        line_number++;
        if (opt_number) {
            printf("%5u  ", line_number);
        }
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        printf("%s%s\n", line, opt_show_ends ? "$" : "");
    }
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
            case 'E': opt_show_ends = true; break;
            case 'n': opt_number = true; break;
            default: break;
        }
    }

    if (optind < argc) {
        int paths_count = argc - optind;
        for (int i = 0; i < paths_count; i++) {
            char* path = argv[optind + i];
            if (!strcmp(path, "-")) {
                // Placeholder for standard input
                print_lines(stdin);
            } else {
                FILE* file = fopen(path, "r");
                if (file == NULL) {
                    dprintf(STDERR_FILENO, "Path '%s' is invalid.\n", path);
                    continue;
                }
                print_lines(file);
                fclose(file);
            }
        }
    } else {
        print_lines(stdin);
    }

    return 0;
}
