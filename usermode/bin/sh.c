#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

#define COMM_BUF_SIZE 256
#define READ_BUF_SIZE 8
#define STDIN_FD 0
#define CURSOR_ENABLE "\033[25h"

#define add_char(c) do {\
    words[curr_word] = realloc(words[curr_word], curr_word_len + 2); \
    words[curr_word][curr_word_len] = c; \
    words[curr_word][curr_word_len + 1] = '\0'; \
    curr_word_len++; } while (0);

#define start_new_line do {\
    words = realloc(words, sizeof(char*) * (curr_word + 3)); \
    words[curr_word + 1] = NULL; \
    words[curr_word + 2] = NULL; \
    curr_word++; \
    curr_word_len = 0; } while (0);

static char comm_buffer[COMM_BUF_SIZE] = {0};
static int comm_cursor_loc = 0;
static int comm_buffer_len = 0;

static void handle_command(void)
{
    if (comm_buffer_len == 0) {
        return;
    }
    printf("\nReceived command: '%s'\n", comm_buffer);
    // NULL-terminated list of words separated by space
    char** words = malloc(sizeof(char*) * 2);
    words[0] = NULL;
    words[1] = NULL;
    int curr_word = 0; // words count = curr_word + 2
    int curr_word_len = 0;

    char in_quote = '\0';
    bool prev_backslash = false;
    for (int i = 0; i < comm_buffer_len; i++) {
        char c = comm_buffer[i];
        if (c == '\\' && !prev_backslash) {
            prev_backslash = true;
            continue;
        }
        else if (prev_backslash) {
            // TODO: parsing different characters? Like new line for example
            add_char(c);
            prev_backslash = false;
            continue;
        }

        if ((c == '"' || c == '\'') && in_quote == '\0')
        {
            // Start in_qoute block
            in_quote = c;
            if (curr_word_len > 0) {
                start_new_line;
            }
            // Initialize an empty word so it's not NULL even if quote is empty
            if (words[curr_word] == NULL) {
                words[curr_word] = malloc(1);
                words[curr_word][0] = '\0';
            }
        }
        else if ((c == '"' || c == '\'') && in_quote == c)
        {
            // End in_qoute block
            in_quote = '\0';
            start_new_line;
        }
        else if (c == ' ' && in_quote != '\0') {
            // Space character, but inside quote
            add_char(' ');
        }
        else if (c == ' ' && curr_word_len == 0) {
            // Starting with space - ignore
            continue;
        }
        else if (c == ' ') {
            // Need to start a new word
            start_new_line;
        }
        else if (c == '|' && in_quote == '\0') {
            // Unescaped pipe outside quotes → standalone token
            if (curr_word_len > 0) {
                start_new_line;
            }
            if (words[curr_word] == NULL) {
                words[curr_word] = malloc(1);
                words[curr_word][0] = '\0';
            }
            add_char('|');
            start_new_line;
        }
        else {
            // Just append character
            add_char(c);
        }
    }
    int words_count = curr_word;
    if (words[curr_word] == NULL) {
        words_count--;
    }
    printf("Words count: %u\n", words_count + 1);
    for (int i = 0; i <= words_count; i++) {
        printf("%u = '%s'\n", i, words[i]);
    }

    // Cleanup
    for (int i = 0; i <= words_count; i++) {
        free(words[i]);
    }
    free(words);
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

static void print_prompt(void)
{
    char* path = "/path"; // TODO
    printf("\n\033[0;96m%s $\033[0m %s", path, CURSOR_ENABLE);
}

int main(void)
{
    // Setup input to disable canon mode
    struct termios termios;
    tcgetattr(STDIN_FD, &termios);
    uint32_t original_lflag = termios.c_lflag; // Keep for later recovery
    termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FD, TCSANOW, &termios);

    // Main command loop
    while (true) {
        print_prompt();
        memset(comm_buffer, 0, COMM_BUF_SIZE);
        comm_cursor_loc = 0;
        comm_buffer_len = 0;

        // Reading loop
        bool continue_reading = true;
        while (continue_reading) {
            char read_buff[READ_BUF_SIZE] = {0};
            int readcount = read(0, read_buff, READ_BUF_SIZE - 1);
            for (int i = 0; i < readcount; i++) {
                char c = read_buff[i];
                if (c == '\n') {
                    continue_reading = false;
                    i = readcount;
                }
                else if (c == '\033') {
                    if (escseq_check(read_buff, readcount, "[D")) {
                        i+=2;
                        if (comm_cursor_loc > 0) {
                            comm_cursor_loc--;
                            printf("\033[D");
                        }
                    }
                    else if (escseq_check(read_buff, readcount, "[C")) {
                        i+=2;
                        if (comm_cursor_loc < comm_buffer_len) {
                            comm_cursor_loc++;
                            printf("\033[C");
                        }
                    }
                    else {
                        i = readcount;
                    }
                }
                else if (comm_cursor_loc < COMM_BUF_SIZE) {
                    if (comm_cursor_loc != comm_buffer_len) {
                        // Shift all characters in the buffer
                        for (int j = comm_buffer_len; j >= comm_cursor_loc; j--)
                        {
                            comm_buffer[j] = comm_buffer[j-1];
                        }
                        // Print all characters
                        putchar(c);
                        for (int j = comm_cursor_loc + 1; j <= comm_buffer_len; j++)
                        {
                            putchar(comm_buffer[j]);
                        }
                        // Return to correct cursor position
                        printf("\033[%uD", comm_buffer_len - comm_cursor_loc);
                    } else {
                        putchar(c);
                    }
                    comm_buffer[comm_cursor_loc] = c;
                    comm_cursor_loc++;
                    comm_buffer_len++;
                }
            }
        }

        handle_command();
    }

    // Bring back the original lflag value
    termios.c_lflag = original_lflag;
    tcsetattr(STDIN_FD, TCSANOW, &termios);
    return 0;
}
