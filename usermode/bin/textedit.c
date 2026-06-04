#include <ctype.h>
#include <errno.h>
#include <limits.h>
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
#define BOTTOM_LINES_RSVD 2
#define LINES_RSVD (TOP_LINES_RSVD + BOTTOM_LINES_RSVD)

// Original termios value, for later recovery
static struct termios orig_termios;
static int term_width = 0;
static int term_height = 0;
// Number of lines available for content
static int content_height = 0;

// Line (in content, 0-indexed)
static int cursor_line = 0;
// Current location of the cursor in the line, 0-indexed
static int cursor_character = 0;

// Line currently displayed at the top of the screen
static int content_scroll = 0;

// Line by line content. All lines include \n and are NULL-terminated.
static char** content = NULL;
static int content_line_count = 0;
static FILE* fstream = NULL;

static char* filename = NULL;
static bool textedit_running = true;

static void textedit_set_cursor(bool enabled)
{
    printf(enabled ? "\033[?25h" : "\033[?25l");
}

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
    printf("\033[%u;1H└", term_height - BOTTOM_LINES_RSVD + 1); // 1-indexed
    for (int i = 1; i < term_width - 1; i++) {
        printf("─");
    }
    printf("┘");
}

static void textedit_rerender_menu(void)
{
    // Navigate to the end of the view
    printf("\033[%u;1H", term_height);
    printf("  \033[1;7m^X\033[22;27m Exit");
    printf("  \033[1;7m^S\033[22;27m Save");
}

/**
 * Re-renders line, zero-indexed, on the screen.
 */
static void textedit_rerender_line(int line)
{
    // 1-indexed, skipping first line for border
    int term_location = line + 1 + TOP_LINES_RSVD;

    if (term_location < (1 + TOP_LINES_RSVD)) {
        // Line too high
        return;
    }
    if (term_location > (term_height - BOTTOM_LINES_RSVD)) {
        // Line too low
        return;
    }
    int content_index = line + content_scroll;
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
    textedit_rerender_menu();
}

/**
 * Move cursor to given line in the content, at the given character.
 */
static void textedit_movecursor(int line, int ch)
{
    // Check borders of the line
    if (line >= content_line_count) {
        line = content_line_count - 1;
    }
    if (line < 0) {
        line = 0;
    }

    if (ch < 0) {
        ch = 0;
    }
    int content_len = strlen(content[line]);
    if (ch >= content_len) {
        ch = content_len > 0 ? content_len - 1 : 0;
    }

    // TODO: check if line is displayed on the screen, and if it doesn't, scroll

    cursor_line = line;
    cursor_character = ch;

    // Move cursor to a valid location in terminal
    int cursor_term_row = cursor_line + TOP_LINES_RSVD;
    // line number + box border + margins
    int cursor_term_col = cursor_character + 7;
    printf(
        "\033[%u;%uH",
        cursor_term_row + 1, // 1-indexed
        cursor_term_col + 1 // 1-indexed
    );
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
    printf("\033[2J\033[7l");
    textedit_set_cursor(false);

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

    // Navigate to the beginning of the content
    textedit_movecursor(0, 0);
    textedit_set_cursor(true);
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
    printf("\033[2J\033[H\033[7h");
    textedit_set_cursor(true);
    // Restore original termios value
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

    // Print content of the file
    for (int i = 0; i < content_line_count; i++) {
        int linelen = strlen(content[i]);
        if (i == content_line_count - 1) {
            // Because it's the last line, the NL is only used to navigate
            // horizontally. We should just skip it.
            content[i][linelen - 1] = '\0';
            linelen--;

            if (linelen == 0) {
                // Last line was only containing the NL at the end
                break;
            }
        }
        printf("Line %u:\n", i);
        for (int j = 0; j < linelen; j++) {
            if (content[i][j] == '\n') printf("<NL>");
            else printf("%c", content[i][j]);
        }
        printf("\n");
    }
}

static bool escseq_check(char* buf, int count, char* test)
{
    int testsize = strlen(test);
    if (testsize > (count - 1)) {
        return false;
    }
    for (int i = 0; i < testsize; i++) {
        if (buf[i + 1] != test[i]) {
            return false;
        }
    }
    return true;
}

static bool textedit_open_file(void)
{
    errno = 0;
    fstream = fopen(filename, "r+");
    if (fstream == NULL && errno == ENOENT) {
        fstream = fopen(filename, "w+");
    }
    return fstream != NULL;
}

static void textedit_save_file(void)
{
    // ls
}

int main(int argc, char** argv)
{
    if (argc <= 1) {
        dprintf(STDERR_FILENO, "textedit requires path to a file.\n");
        return 1;
    } else {
        filename = malloc(sizeof(char) * (strlen(argv[1]) + 1));
        strcpy(filename, argv[1]);
        if (!textedit_open_file()) {
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

        // If file ended with newline, we should append a synthetic empty line
        if (content_line_count > 0) {
            char* last = content[content_line_count - 1];
            int len = strlen(last);
            if (len > 0 && last[len-1] == '\n') {
                content_line_count++;
                content = realloc(content, sizeof(char*) * content_line_count);
                content[content_line_count - 1] = strdup("\n");
            }
        }
    }

    if (!textedit_init()) {
        return 3;
    }

    struct sigaction act;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    sigaction(SIGINT, &act, NULL);

    char read_buff[16] = {0};
    while (textedit_running) {
        // Reading loop
        int readcount = read(STDIN_FILENO, read_buff, 15);
        for (int i = 0; i < readcount; i++) {
            char c = read_buff[i];
            if (c == '\033') {
                if (escseq_check(read_buff, readcount, "[A")) {
                    // arrow up
                    i += 2;
                    textedit_movecursor(cursor_line - 1, cursor_character);
                }
                else if (escseq_check(read_buff, readcount, "[B")) {
                    // arrow down
                    i += 2;
                    textedit_movecursor(cursor_line + 1, cursor_character);
                }
                else if (escseq_check(read_buff, readcount, "[C")) {
                    // arrow right
                    i += 2;
                    textedit_movecursor(cursor_line, cursor_character + 1);
                }
                else if (escseq_check(read_buff, readcount, "[D")) {
                    // arrow left
                    i += 2;
                    textedit_movecursor(cursor_line, cursor_character - 1);
                }
                else if (escseq_check(read_buff, readcount, "[H")) {
                    // Home
                    i += 2;
                    textedit_movecursor(cursor_line, 0);
                }
                else if (escseq_check(read_buff, readcount, "[F")) {
                    // End
                    i += 2;
                    textedit_movecursor(cursor_line, INT_MAX);
                }
            }
            else if (c == '\023') { // Ctrl+S - saving
                textedit_save_file();
            }
            else if (c == '\030') { // Ctrl+X - exit
                textedit_running = false;
            }
            else if (c == '\b') {
                if (cursor_character > 0) {
                    // Move all characters back
                    int linelen = strlen(content[cursor_line]);
                    for (int j = cursor_character - 1; j < linelen; j++) {
                        content[cursor_line][j] = content[cursor_line][j+1];
                    }
                    content[cursor_line][linelen] = '\0';
                    // Rerender line
                    textedit_set_cursor(false);
                    textedit_rerender_line(
                        cursor_line - content_scroll
                    );
                    textedit_movecursor(cursor_line, cursor_character - 1);
                    textedit_set_cursor(true);
                }
                else if (cursor_line > 0) {
                    // Remove newline at the end of previous line
                    int prevline_len = strlen(content[cursor_line - 1]);
                    content[cursor_line - 1][prevline_len - 1] = '\0';
                    // Extend the size of previous line, to be able to include
                    // the current line
                    int linelen = strlen(content[cursor_line]);
                    content[cursor_line - 1] = realloc(
                        content[cursor_line - 1],
                        sizeof(char) * (prevline_len + linelen + 1)
                    );
                    // Merge lines
                    strcat(content[cursor_line - 1], content[cursor_line]);
                    // Delete current line
                    free(content[cursor_line]);
                    // Move all lines below one step up
                    for (int r = cursor_line; r < content_line_count - 1; r++)
                    {
                        content[r] = content[r + 1];
                    }
                    content[content_line_count - 1] = NULL;
                    content_line_count--;
                    // Rerender 1 line above + all lines below
                    textedit_set_cursor(false);
                    int curr_line_terminal = cursor_line - content_scroll - 1;
                    for (int li = curr_line_terminal; li < content_height; li++)
                    {
                        textedit_rerender_line(li);
                    }
                    textedit_movecursor(cursor_line - 1, prevline_len - 1);
                    textedit_set_cursor(true);
                }
            }
            else if (c == '\n') {
                char* newline_content = strdup(
                    content[cursor_line] + cursor_character
                );
                // Allocate space for one more line
                content_line_count++;
                content = realloc(content, sizeof(char*) * content_line_count);
                // Move pointers to all lines below
                for (int j = content_line_count; j > cursor_line; j--) {
                    content[j] = content[j-1];
                }
                // Insert new line in the slot
                content[cursor_line + 1] = newline_content;

                // Set current character to \n and remove all characters after
                content[cursor_line][cursor_character] = '\n';
                int line_len = strlen(content[cursor_line]);
                for (int j = cursor_character + 1; j < line_len; j++) {
                    content[cursor_line][j] = '\0';
                }

                // Re-render current line and all lines below
                textedit_set_cursor(false);
                int curr_line_terminal = cursor_line - content_scroll;
                for (int li = curr_line_terminal; li < content_height; li++) {
                    textedit_rerender_line(li);
                }
                textedit_movecursor(cursor_line + 1, 0);
                textedit_set_cursor(true);
            }
            else if (isprint(c)) {
                // Printable character - add it at current location.
                // First, reallocate to add slot for one more character
                int linelen = strlen(content[cursor_line]) + 1;
                content[cursor_line] = realloc(
                    content[cursor_line],
                    linelen + 1
                );
                // Shift all characters
                for (int j = linelen; j > cursor_character; j--) {
                    content[cursor_line][j] = content[cursor_line][j-1];
                }
                // Place new character at chosen place
                content[cursor_line][cursor_character] = c;
                // Rerender line
                textedit_set_cursor(false);
                textedit_rerender_line(
                    cursor_line - content_scroll
                );
                textedit_movecursor(cursor_line, cursor_character + 1);
                textedit_set_cursor(true);
            }
        }
    }

    textedit_close();
    return 0;
}
