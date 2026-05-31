#ifndef _SIGNAL_H
#define _SIGNAL_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stdint.h>

typedef uint32_t sigset_t;

// SIGNAL VALUES DEFINITIONS
// Most of them are not currently used by kernel in any way, unless explicitly
// stated in docs.

#define SIGHUP 1
// Send to the foregroud task by the terminal when user presses Ctrl+C
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGSTKFLT 16
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
// Send to the foregroud task by the terminal when user presses Ctrl+Z
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGURG 23
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGWINCH 28
#define SIGIO 29
#define SIGPWR 30
#define SIGSYS 31

/**
 * When set, use sa_sigaction instead of sa_handler.
 */
#define SA_SIGINFO 0x01

/**
 * When set, do not automatically add signal to the signal mask while the
 * handler is executing.
 */
#define SA_NODEFER 0x02

/**
 * Launch given handler only once, then reset the signal handler to the default
 * state.
 */
#define SA_RESETHAND 0x04

/**
 * Value to be passed as `sa_handler` to bring handler to the default one.
 */
#define SIG_DFL ((void(*)(int))0)

/**
 * Value to be passed as `sa_handler` to remove signal handler.
 */
#define SIG_IGN ((void(*)(int))1)

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
     * A simple handler, that only receives a signal number in argument. Can
     * be set to SIG_DFL or SIG_IGN as well.
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

/**
 * For given signal number `signum`:
 * - if `act` is not NULL - updates the signal handler for `signum`
 * - if `oldact` is not NULL - writes previous signal handler in `oldact`
 *
 * On success returns 0. On failure returns -1 and sets `errno` value.
 */
int sigaction(
    int signum,
    const struct sigaction* restrict act,
    struct sigaction* restrict oldact
);

/**
 * Sends signal `sig` to process `pid`.
 *
 * If `pid` is equal to 0, then signal is sent to every process in the same
 * process group as the current process.
 *
 * If `pid` is less than -1, then signal is sent to every process in the process
 * group with ID equal to `-pid`.
 *
 * Returns 0 on success; on failure returns
 * -1 and sets `errno` value to one of following values:
 * - EINVAL - an invalid signal was specified
 * - ESRCH - the target process (or process group) doesn't exist
 */
int kill(pid_t pid, int sig);

#endif //#if !(defined(__is_libk))

#ifdef __cplusplus
}
#endif

#endif
