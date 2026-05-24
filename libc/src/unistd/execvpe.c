#if !(defined(__is_libk))

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <unistd.h>

#define PATH_ENV_DEFAULT "/bin:/usr/bin:/initrd/bin"

int execvpe(const char* path, const char* argv[], const char* envp[])
{
    if (strchr(path, '/'))
        return execve(path, argv, envp);

    const char* path_env = NULL;
    if (envp) {
        for (int i = 0; envp[i]; i++) {
            if (strncmp(envp[i], "PATH=", 5) == 0) {
                path_env = envp[i] + 5;
                break;
            }
        }
    }
    if (!path_env) {
        path_env = PATH_ENV_DEFAULT;
    }

    size_t path_len = strlen(path);
    int last_errno = ENOENT;

    const char* dir = path_env;
    while (true) {
        const char* colon = strchr(dir, ':');
        size_t dir_len = colon ? (size_t)(colon - dir) : strlen(dir);

        char* full = malloc(dir_len + 1 + path_len + 1);
        if (!full) {
            errno = ENOMEM;
            return -1;
        }

        if (dir_len == 0) {
            full[0] = '.';
            full[1] = '/';
            memcpy(full + 2, path, path_len + 1);
        } else {
            memcpy(full, dir, dir_len);
            full[dir_len] = '/';
            memcpy(full + dir_len + 1, path, path_len + 1);
        }

        execve(full, argv, envp);
        // On success execve never returns. If we reached here, that means that
        // program was not found in this path and we can continue with next one.
        last_errno = errno;
        free(full);

        if (!colon) {
            break;
        }
        dir = colon + 1;
    }

    errno = last_errno;
    return -1;
}

#endif
