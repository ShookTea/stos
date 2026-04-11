#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define PATH "/dev/hda"
#define SIZE_BYTES 1024

int main(void)
{
    int fd = open(PATH, O_RDONLY);
    printf("FD = %d", fd);
    uint8_t* data = malloc(SIZE_BYTES);
    memset(data, 0, SIZE_BYTES);
    read(fd, data, SIZE_BYTES);
    for (size_t i = 0; i < SIZE_BYTES; i++) {
        if (i % 16 == 0) {
            puts("");
        }
        printf("%02X ", data[i]);
    }
    puts("");
    close(fd);

    return 0;
}
