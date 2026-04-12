#if !(defined(__is_libk))

#include <stdarg.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

int execle(
    const char* path,
    const char* arg,
    ... /* , NULL, const char* envp[] */
) {
    va_list args;
    va_start(args, arg);
    char** argv = NULL;
    size_t index = 0;
    const char* curr_arg = arg;
    while (curr_arg != NULL) {
        argv = realloc(argv, sizeof(char*) * (index + 1));
        argv[index] = (char*)curr_arg;
        index++;
        curr_arg = va_arg(args, const char*);
    }
    // Add null as a terminator
    argv = realloc(argv, sizeof(char*) * (index + 1));
    argv[index] = NULL;

    const char** envp = va_arg(args, const char**);
    va_end(args);

    int ret = execve(path, (const char**)argv, envp);
    free(argv);
    return ret;
}

#endif
