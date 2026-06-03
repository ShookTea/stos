#include <signal.h>
#include <sys/types.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>

// 1 line reserved for top box line
#define TOP_LINES_RSVD 1
// 1 line reserved for bottom box line
#define BOTTOM_LINES_RSVD 1
#define LINES_RSVD (TOP_LINES_RSVD + BOTTOM_LINES_RSVD)

// Original termios value, for later recovery
static struct termios orig_termios;
static int term_width = 0;
static int term_height = 0;
// Number of lines available for content
static int content_height = 0;

// Line currently displayed at the top of the screen
static int top_line_index = 0;

// Line by line content. All lines include \n and are NULL-terminated.
static char** content = NULL;
static int content_line_count = 0;
static FILE* fstream;

static char* filename = NULL;

/**
 * Renders border around the editor
 */
static void textedit_rerender_border(void)
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

    // Print borders on the left and right for each line
    for (int i = 1; i < term_height - BOTTOM_LINES_RSVD; i++) {
        printf("\033[%u;1H│\033[%uG│", i + 1, term_width);
    }

    // Print bottom line
    printf("\033[%u;1H└", term_height);
    for (int i = 1; i < term_width - 1; i++) {
        printf("─");
    }
    printf("┘");
}

/**
 * Re-renders line, zero-indexed, on the screen.
 */
static void textedit_rerender_line(int line)
{
    int term_location = line + 2; // 1-indexed, skipping first line for border

    if (term_location < (1 + TOP_LINES_RSVD)) {
        // Line too high
        return;
    }
    if (term_location > (term_height - BOTTOM_LINES_RSVD)) {
        // Line too low
        return;
    }
    int content_index = line + top_line_index;
    // Navigate to the beginning of the line, after box border
    printf("\033[%u;2H", term_location);

    if (content_index >= content_line_count) {
        // Line is present in the screen, but not in the content.
        // Clear the line
        printf("\033[K");
        // Redraw box line at the right
        printf("\033[%uG│", term_width);
        return;
    } else {

    }

    // Line is present in the content. Print line number, 1-indexed:
    // (max 4 digits, padded, + 2 spaces as a margin)
    printf(" \033[90m%4u\033[m ", content_index + 1);

    // Remaining space: terminal width - 6 characters from above - 2 box lines
    int remaining_space = term_width - 8;
    char format[10] = {0};
    sprintf(format, "%%-.%us", remaining_space);

    // Print line with format. Remove newline at the end, if present.
    char* to_print = strdup(content[content_index]);
    int len = strlen(to_print);
    if (to_print[len - 1] == '\n') {
        to_print[len - 1] = '\0';
    }
    printf(format, to_print);
    free(to_print);

    // Clear the rest of the line
    printf("\033[K");
    // Redraw box line at the right
    printf("\033[%uG│", term_width);
}

// Re-renders the entire screen
static void textedit_rerender_all(void)
{
    textedit_rerender_border();
    for (int i = 0; i <= content_height; i++) {
        textedit_rerender_line(i);
    }
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
        content_height = term_height - LINES_RSVD; // 2 lines of border
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
