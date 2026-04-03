#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd = open("/dev/tty", O_RDONLY);
    printf("FD = %d\n", fd);
    uint8_t* data = malloc(32);
    read(fd, data, 32);
    data[31] = 0;
    char* text = (char*)data;
    printf(">%s<\n", text);
    close(fd);

    return 0;
}
