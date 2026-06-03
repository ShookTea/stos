#include <signal.h>
#include <sys/types.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>

// Original termios value, for later recovery
static struct termios orig_termios;
static int term_width = 0;
static int term_height = 0;

// Line by line content. All lines include \n and are NULL-terminated.
static char** content = NULL;
static int content_line_count = 0;
static FILE* fstream;

static char* filename = NULL;

// Re-renders the entire screen
static void textedit_rerender_all(void)
{
    // Navigate to the beginning of the screen
    printf("\033[H");

    // Print top line, with filename
    printf("┌──");
    int printed = 3;
    printed += printf(" %s ", filename);
    for (int i = printed; i < term_width - 1; i++) {
        printf("─");
    }
    printf("┐");

    // Print main content lines
    for (int i = 1; i < term_height - 1; i++) {
        // Print left line + color for line numbering
        printf("\033[%u;1H│\033[90m ", i + 1);
        if (i <= content_line_count) {
            printf("%4u", i);
        } else {
            printf("    ");
        }

        // End line numbering color
        printf(" \033[m");

        // 1 character for border + 4 numline characters + 2 spaces
        int printed_in_line = 7;
        int remaining_space = (term_width - 1) - printed_in_line;
        char format[10] = {0};
        sprintf(format, "%%-.%us", remaining_space);
        if (i <= content_line_count) {
            char* line = content[i - 1];
            int len = strlen(line);
            if (line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            printed_in_line += printf(format, content[i - 1]);
        }
        for (int j = printed_in_line; j < term_width - 1; j++) {
            printf(" ");
        }
        printf("│");
    }

    // Print bottom line
    printf("\033[%u;1H└", term_height);
    for (int i = 1; i < term_width - 1; i++) {
        printf("─");
    }
    printf("┘");
}

static bool textedit_init(void)
{
    // Load termios config and store backup
    struct termios termios;
    tcgetattr(STDIN_FILENO, &termios);
    memcpy(&orig_termios, &termios, sizeof(struct termios));

    // Set flags
    termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &termios);

    // Clear terminal, disable autowrapping and cursor
    printf("\033[2J\033[7l\033[?25l");
    // Measure screen size

    printf("\033[9999;9999H\033[6n");
    char buf[32] = {0};
    int read_count = read(STDIN_FILENO, buf, 31);
    if (read_count > 0 && buf[0] == '\033' && buf[1] == '[') {
        char* ptr = buf + 2; // skip start
        ptr[strlen(ptr) - 1] = '\0';
        term_height = atoi(strtok(ptr, ";"));
        term_width = atoi(strtok(NULL, ";"));
    } else {
        return false;
    }

    textedit_rerender_all();
    return true;
}

static void textedit_close(void)
{
    free(filename);
    for (int i = 0; i < content_line_count; i++) {
        free(content[i]);
    }
    free(content);
    fclose(fstream);
    // Clean screen, reset cursor position, reenable autowrapping and cursor
    printf("\033[2J\033[H\033[7h\033[?25h");
    // Restore original termios value
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

static void sigint_handler(int signum __attribute__((unused)))
{
    // TODO - show the confirmation
}

int main(int argc, char** argv)
{
    if (argc <= 1) {
        dprintf(STDERR_FILENO, "textedit requires path to a file.\n");
        return 1;
    } else {
        filename = malloc(sizeof(char) * (strlen(argv[1]) + 1));
        strcpy(filename, argv[1]);
        fstream = fopen(filename, "r+");
        if (fstream == NULL) {
            dprintf(STDERR_FILENO, "Error when opening file %s\n", filename);
            free(filename);
            return 2;
        }

        size_t n = 0;
        ssize_t read_bytes;
        char* line = NULL;
        while ((read_bytes = getline(&line, &n, fstream)) >= 0) {
            content_line_count++;
            content = realloc(content, sizeof(char*) * content_line_count);
            content[content_line_count - 1] = line;
            line = NULL;
            n = 0;
        }
        free(line); // In case getline allocated a buffer before hitting EOF
    }

    if (!textedit_init()) {
        return 3;
    }

    struct sigaction act;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    act.sa_handler = sigint_handler;
    sigaction(SIGINT, &act, NULL);

    int c = fgetc(stdin);
    textedit_close();
    printf("c = %c\n", c);
    return 0;
}
