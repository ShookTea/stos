#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define SIZE_BYTES 1024

int main(int argc, char** argv)
{
    if (argc < 2) {
        puts("hexdump requires path as an argument.");
        return 1;
    }
    char* path = argv[1];
    printf("path = %s\n", path);
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        puts("Error when running open()");
        return 2;
    }

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
