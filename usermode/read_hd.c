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
    printf("FD = %d\n", fd);
    uint8_t* data = malloc(SIZE_BYTES);
    memset(data, 0, SIZE_BYTES);
    int read_bytes = read(fd, data, SIZE_BYTES);
    printf("%u bytes read", read_bytes);
    for (size_t i = 0; i < SIZE_BYTES; i++) {
        if (i % 32 == 0) {
            puts("");
        }
        printf("%02X ", data[i]);
    }
    puts("");
    close(fd);

    return 0;
}
