#include <termios.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd = open("/dev/tty", O_RDONLY);
    if (fd == -1) {
        puts("/dev/tty file not found");
    }

    uint8_t* data = malloc(32);

    struct termios termios;
    tcgetattr(fd, &termios);
    uint32_t original_lflag = termios.c_lflag; // Keep for later recovery

    // Set the ECHO flag on
    termios.c_lflag |= ECHO;
    tcsetattr(fd, TCSANOW, &termios);

    printf("Input (visible): ");
    read(fd, data, 32);
    data[31] = 0;
    char* text = (char*)data;
    printf("Result: %s", text);

    // Set the ECHO flag off
    termios.c_lflag &= ~ECHO;
    tcsetattr(fd, TCSANOW, &termios);

    printf("Input (hidden): ");
    read(fd, data, 32);
    data[31] = 0;
    text = (char*)data;
    printf("\nResult: %s", text);

    // Restore lflag
    termios.c_lflag = original_lflag;
    tcsetattr(fd, TCSANOW, &termios);

    close(fd);
    free(data);

    return 0;
}
