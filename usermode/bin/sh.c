#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

#define COMM_BUF_SIZE 256
#define READ_BUF_SIZE 8
#define STDIN_FD 0
#define CURSOR_ENABLE "\033[25h"

static char comm_buffer[COMM_BUF_SIZE] = {0};
static int comm_cursor_loc = 0;
static int comm_buffer_len = 0;

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

    while (true) {
        // Main command loop
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
    }

    // Bring back the original lflag value
    termios.c_lflag = original_lflag;
    tcsetattr(STDIN_FD, TCSANOW, &termios);
    return 0;
}
