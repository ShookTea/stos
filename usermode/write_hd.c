#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define PATH "/dev/hda"
#define SIZE_BYTES 4

int main(void)
{
    int fd = open(PATH, O_WRONLY);
    printf("FD = %d\n", fd);
    uint8_t* data = malloc(SIZE_BYTES);
    memset(data, 0, SIZE_BYTES);
    data[0] = 0xCA;
    data[1] = 0xFE;
    data[2] = 0xBA;
    data[3] = 0xBE;
    size_t res = write(fd, data, SIZE_BYTES);
    printf("Writing completed - %u bytes written\n", res);
    close(fd);

    return 0;
}
