#include <sys/syscall.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main(void)
{
    errno = 0;
    int fd = open("/dev", O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        printf("errno = %u\n", errno);
        return errno;
    }

    int syscall_res = 0;
    struct dirent dir[2];
    do {
        syscall_res = syscall(SYS_GETDENTS, fd, (int)&dir, 2);
        if (syscall_res < 0) {
            printf("syscall errno = %u\n", -syscall_res);
            close(fd);
            return -syscall_res;
        }
        for (int i = 0; i < syscall_res; i++) {
            printf("[%c] '%s'\n", dir[i].d_type, dir[i].d_name);
        }
    } while (syscall_res > 0);

    return 0;
}
