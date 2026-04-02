#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd = open("/dev/tty", O_RDONLY);
    printf("FD = %d\n", fd);
    uint8_t* data = malloc(16);
    int readcount = 0;
    while ((readcount = read(fd, data, 16)) != 0) {
        for (int i = 0; i < readcount; i++) {
            printf("%02x", data[i]);
            if (i % 2 == 1) {
                printf(" ");
            }
        }
        printf("\n");
    }

    return 0;
}
