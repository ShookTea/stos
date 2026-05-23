#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

#define COMM_BUF_SIZE 256
#define STDIN_FD 0
#define CURSOR_ENABLE "\033[25h"

static char comm_buffer[COMM_BUF_SIZE] = {0};

int main(void)
{
    // Setup input to disable canon mode
    struct termios termios;
    tcgetattr(STDIN_FD, &termios);
    uint32_t original_lflag = termios.c_lflag; // Keep for later recovery
    termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FD, TCSANOW, &termios);

    printf(CURSOR_ENABLE);
    while (true) {
        memset(comm_buffer, 0, COMM_BUF_SIZE);
        int readcount = read(0, comm_buffer, COMM_BUF_SIZE - 1);
        if (comm_buffer[0] != '\033') {
            printf("\nrc=%u, out='%s'\n", readcount, comm_buffer);
        } else {
            comm_buffer[0] = '^';
            printf("\nrc=%u, out=esc'%s'\n", readcount, comm_buffer);
        }
    }

    // Bring back the original lflag value
    termios.c_lflag = original_lflag;
    tcsetattr(STDIN_FD, TCSANOW, &termios);
    return 0;
}
