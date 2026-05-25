#include <dirent.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main(void)
{
    // Opening pipe descriptors
    int pipefd[2];
    errno = 0;
    if (pipe(pipefd) < 0) {
        printf("pipe() errno = %u\n", errno);
        return 1;
    }

    printf("pipefd[0] = %u, pipefd[1] = %u\n", pipefd[0], pipefd[1]);



    int pid = fork();
    // Check list of open file descriptors
    DIR* dirp = opendir("/proc/self/fd");
    struct dirent* dir;
    while ((dir = readdir(dirp)) != NULL) {
        printf("- '%s'\n", dir->d_name);
    }
    closedir(dirp);
    printf("pid = %u\n", pid);

    if (pid == 0) {
        // Child process will try to write
        errno = 0;
        int write_count = write(pipefd[1], "Hello, world!", 14);
        if (write_count < 0) {
            printf("(pid=%u) write errno=%u\n", pid, errno);
        } else {
            printf("(pid=%u) write count = %u\n", pid, write_count);
        }
    } else {
        // Parent process will try to read
        errno = 0;
        char output[14] = {0};
        int read_count;
        while ((read_count = read(pipefd[0], output, 14)) == 0) {};
        if (read_count < 0) {
            printf("(pid=%u) read errno=%u\n", pid, errno);
        } else {
            printf("(pid=%u) read count = %u, output = '%s'\n", pid, read_count, output);
        }
        waitpid(pid, NULL, 0);
    }

    // Closing
    int result = 0;
    errno = 0;
    if (close(pipefd[0]) < 0) {
        printf("close(pipefd[0]) errno = %u\n", errno);
        result = 1;
    }
    errno = 0;
    if (close(pipefd[1]) < 0) {
        printf("close(pipefd[1]) errno = %u\n", errno);
        result = 1;
    }

    return result;
}
