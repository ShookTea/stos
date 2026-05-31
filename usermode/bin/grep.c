#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#define LINE_BUF 4096

#define OPT_SHORT "vclLhHn"

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
    { "invert-match", no_argument, NULL, 'v' },
    { "count", no_argument, NULL, 'c' },
    { "color", optional_argument, NULL, 0 },
    { "files-with-matches", no_argument, NULL, 'l' },
    { "files-without-match", no_argument, NULL, 'L' },
    { "no-filename", no_argument, NULL, 'h' },
    { "with-filename", no_argument, NULL, 'H' },
    { "label", required_argument, NULL, 0 },
    { "line-number", no_argument, NULL, 'n' },
    { NULL, 0, NULL, 0 },
};

static void print_usage(void)
{
    puts("Usage:");
    puts("  grep [options] <pattern> [files...]");
    puts("");
    puts("META OPTIONS");
    puts("  --help                      Shows usage info");
    puts("");
    puts("MATCHING OPTIONS");
    puts("  -v, --invert-match          Invert matching, printing only non-matched lines");
    puts("");
    puts("GENERAL OUTPUT OPTIONS");
    puts("  -c, --count                 Instead of matched lines, print count of matched lines for each file.");
    puts("  --color[=WHEN]              Enables/disables coloring (more below)");
    puts("  -l, --files-with-matches    Instead of matched lines, print names of files with matched line. End scanning after first match.");
    puts("  -L, --files-without-match   Instead of matched lines, print names of files with no matched line.");
    puts("");
    puts("OUTPUT LINE PREFIX OPTIONS");
    puts("  -h, --no-filename           Don't add file names before match. It's default if only one file (or only standard input) is used.");
    puts("  -H, --with-filename         Add file names before match. It's default if more than one file is used.");
    puts("  --label=LABEL               When printing filenames, use given LABEL for standard input.");
    puts("  -n, --line-number           Prefix each line with 1-based line number within the input file");
    puts("");
    puts("ARGUMENTS");
    puts("  <pattern>                   Pattern used for searching in the input.");
    puts("  [files...]                  One or more files to search for the pattern. \"-\" stands for standard input.");
    puts("                              If no files are given, it will default to standard input.");
    puts("");
    puts("DETAILS");
    puts("  --color[=WHEN] enables or disables coloring. By default, coloring is disabled. \"WHEN\" can equal \"auto\", \"always\", or \"never\".");
    puts("  If --color is passed, but without value, it is treated as \"always. If set to \"auto\", it will only apply coloring when output is");
    puts("  connected to TTY.");
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
static bool prefix_filenames = false;
static bool prefix_line_number = false;
static bool enable_colors = false;

static void print_match(
    const char* line,
    const char* match,
    const char* filename,
    int line_number,
    int match_length
) {
    if (prefix_filenames) {
        printf(
            "%s%s%s:%s",
            enable_colors ? "\033[95m" : "",
            filename,
            enable_colors ? "\033[96m" : "",
            enable_colors ? "\033[0m" : ""
        );
    }
    if (prefix_line_number) {
        printf(
            "%s%u%s:%s",
            enable_colors ? "\033[93m" : "",
            line_number,
            enable_colors ? "\033[96m" : "",
            enable_colors ? "\033[0m" : ""
        );
    }
    if (match == NULL) {
        // match not found in the line - invert was used. Just print the line
        printf(line);
        return;
    }

    // Match found in the line. If colors are not enabled, just print the
    // whole line.
    if (!enable_colors) {
        printf(line);
        return;
    }

    // Colors enabled - first print the part before the match
    int line_length = strlen(line);
    int substr_pos = match - line;
    int substr_end = match_length + substr_pos;
    int after_substr_len = line_length - substr_end;
    char pattern[128] = {0};
    sprintf(
        pattern,
        "%%.%us\033[1;91m%%.%us\033[0m%%.%us",
        substr_pos,
        match_length,
        after_substr_len
    );
    printf(
        pattern,
        line,
        line + substr_pos,
        line + substr_pos + match_length
    );
}

/**
 * Tries to find pattern in the line. If found, returns pointer to the beginning
 * of the match and sets `match_len` to length of the match; otherwise returns
 * NULL.
 */
static char* find_match(const char* line, const char* pattern, int* match_len)
{
    char* found_match = strstr(line, pattern);
    if (found_match != NULL) {
        *match_len = strlen(pattern);
        return found_match;
    }
    return NULL;
}

static void run_grep(
    const char* pattern,
    FILE* file,
    const char* filename
) {
    char line[LINE_BUF];
    int matched_count = 0;
    int match_length = 0;
    int line_number = 0;
    bool print_matched = !print_count_only
        && !print_files_with_match_only
        && !print_files_without_match_only;

    while (fgets(line, LINE_BUF, file)) {
        line_number++;
        char* found_match = find_match(line, pattern, &match_length);
        bool found = found_match != NULL;
        if (found != invert_match) {
            matched_count++;
            if (print_files_without_match_only || print_files_with_match_only) {
                break;
            }
            if (print_matched) {
                print_match(
                    line,
                    found_match,
                    filename,
                    line_number,
                    match_length
                );
            }
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
    bool with_filename_opt = false;
    bool no_filename_opt = false;
    char stdin_label[256] = "(standard input)";
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
                else if (!strcmp(opts[option_index].name, "label")) {
                    strcpy(stdin_label, optarg);
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
                            "grep: option --color has no \"%s\" setting",
                            optarg
                        );
                        return 1;
                    }
                }
                break;
            }
            case 'v': invert_match = true; break;
            case 'c': print_count_only = true; break;
            case 'l': print_files_with_match_only = true; break;
            case 'L': print_files_without_match_only = true; break;
            case 'h': no_filename_opt = true; break;
            case 'H': with_filename_opt = true; break;
            case 'n': prefix_line_number = true; break;
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
        prefix_filenames = (files_count > 1 && !no_filename_opt)
            || (files_count == 1 && with_filename_opt);
        for (int i = 0; i < files_count; i++) {
            char* path = argv[optind + i + 1];
            if (!strcmp(path, "-")) {
                // placeholder for standard input
                run_grep(pattern, stdin, stdin_label);
            } else {
                FILE* file = fopen(path, "r");
                if (file == NULL) {
                    dprintf(STDERR_FILENO, "Path '%s' is invalid.\n", path);
                    continue;
                }
                run_grep(pattern, file, path);
                fclose(file);
            }
        }
    } else {
        // Only standard input used
        prefix_filenames = with_filename_opt;
        run_grep(pattern, stdin, stdin_label);
    }

    return 0;
}
