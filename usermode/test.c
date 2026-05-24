#include <sys/syscall.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main(void)
{
    errno = 0;
    DIR* dir = opendir("/dev");
    if (errno != 0) {
        printf("opendir errno=%u\n", errno);
        return errno;
    }

    struct dirent* dirent;
    while ((dirent = readdir(dir)) != NULL) {
        printf("[%c] '%s'\n", dirent->d_type, dirent->d_name);
    }

    if (errno != 0) {
        printf("readdir errno=%u\n", errno);
    }

    closedir(dir);

    return 0;
}
