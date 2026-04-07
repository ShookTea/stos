#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H 1
#include <sys/cdefs.h>

#if !(defined(__is_libk))

#ifdef __cplusplus
extern "C" {
#endif

#define WNOHANG 1

/**
 * Suspends execution of current task until any child of this tasks finishes
 * execution, and returns PID of that task.
 *
 * Calling this function is equivalent to `waitpid(-1, status_code, 0)`.
 */
int wait(int* status_code);

/**
 * Suspends execution of current task until task with given PID stops execution
 * and is moved into ZOMBIE state.
 *
 * If PID is set to value greater than 0, waitpid will wait for a task with
 * specified PID; otherwise, it will wait for any child process of the current
 * task.
 *
 * If status_code is not NULL, the final status code of the task will be stored
 * in given pointer.
 *
 * "options" consists of flags that define special behavior. There is currently
 * one flag, WNOHANG, which will cause the function to return immediately
 * instead of hanging if specified child has not exited.
 *
 * In case of failure, this function will return -1. In case the function
 * worked correctly and a child process has completed work, it will return the
 * PID of that task. In addition, in case WNOHANG option is used:
 * - the function will return -1 if current process doesn't have child with
 *   specified PID (if specified)
 * - the function will return 0 if current process has a child with specified
 *   PID (or any child, if PID is not specified), but has not stopped yet.
 */
int waitpid(int pid, int* status_code, int options);

#ifdef __cplusplus
}
#endif

#endif
#endif
