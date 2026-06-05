#if !(defined(__is_libk))

#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>

#define SHEBANG_BUFF_SIZE 128

int execve(
    const char* path,
    const char* argv[],
    const char* envp[]
) {
    // First, try to read content of the file - if it's a script, starting with
    // shebang.
    // TODO: test if given path is executable
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        // Error was already set by `open`.
        return -1;
    }
    char buf[SHEBANG_BUFF_SIZE] = {0};
    read(fd, buf, SHEBANG_BUFF_SIZE - 1);
    if (buf[0] == '#' && buf[1] == '!') {
        char* shebang = buf + 2;
        for (int i = 0; i < SHEBANG_BUFF_SIZE - 2; i++) {
            if (shebang[i] == '\n') {
                shebang[i] = '\0';
                break;
            }
        }

        // `shebang` now contains path to executable. We should include path
        // to this file as an argument.
        // TODO: handle cases where shebang has both path and optional args:
        // they should prepend CLI args.
        argv[0] = path;
        path = shebang;
    }

    // Run standard syscall
    int res = (int)syscall(SYS_EXEC, (int)path, (int)argv, (int)envp);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return 0;
}

#endif
