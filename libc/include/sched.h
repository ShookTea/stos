#ifndef _SCHED_H
#define _SCHED_H 1
#include <sys/cdefs.h>

#if !(defined(__is_libk))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Yield the processor, moving the current task to the end of the queue.
 * Returns 0 on success and -1 on error.
 */
int sched_yield();

#ifdef __cplusplus
}
#endif

#endif
#endif
