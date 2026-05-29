#ifndef _SIGNAL_H
#define _SIGNAL_H 1
#include <sys/cdefs.h>
#include <stdint.h>

typedef uint32_t sigset_t;

/**
 * Data structure passed to `sa_sigaction`. si_signo, si_errno and si_code are
 * defined for all signals; other fields are defined only for specific signals
 * and may contain unexpected values for non-supported signals.
 */
typedef struct {
    int si_signo;
    int si_errno;
    int si_code;
} siginfo_t;

/**
 * Data structure for storing expected action to a signal. `sa_handler` and
 * `sa_sigaction` both define the action taken when receiving a signal, but only
 * one of them is used at the same time, depending on whether `SA_SIGINFO` flag
 * was used.
 */
struct sigaction {
    /**
     * A simple handler, that only receives a signal number in argument.
     */
    void (*sa_handler)(int);
    /**
     * More complex handler, that receives:
     * - signal number
     * - pointer to `siginfo_t`
     * - user context (currently always NULL)
     */
    void (*sa_sigaction)(int, siginfo_t*, void*);
    /**
     * Which signals should be surpressed when this signal handler is called?
     */
    sigset_t sa_mask;
    /**
     * Additional flags, starting with `SA_` prefix.
     */
    int sa_flags;
    /**
     * Currently not used.
     */
    void (*sa_restorer)(void);
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the signal `set` to empty, with all signals excluded from the
 * list. Returns 0 on success and -1 on failure.
 */
int sigemptyset(sigset_t* set);

/**
 * Initializes the signal `set` to full, with all signals included in the
 * list. Returns 0 on success and -1 on failure.
 */
int sigfillset(sigset_t* set);

/**
 * Adds signal `signum` to `set`. Returns 0 on success and -1 on failure.
 */
int sigaddset(sigset_t* set, int signum);

/**
 * Removes signal `signum` from `set`. Returns 0 on success and -1 on failure.
 */
int sigdelset(sigset_t* set, int signum);

/**
 * Checks if signal `signum` is enabled in `set`. Returns 1 if signal is
 * present, 0 if not present, and -1 on error.
 */
int sigismember(const sigset_t* set, int signum);

#if !(defined(__is_libk))
// Functions for tasks will live here
#endif

#ifdef __cplusplus
}
#endif

#endif
