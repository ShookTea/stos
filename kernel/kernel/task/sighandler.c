#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/memory/vmm.h"
#include <sys/syscall.h>
#include <signal.h>

typedef enum { SIG_ACTION_TERMINATE, SIG_ACTION_IGNORE } sig_default_action_t;

// Indices into the uint32_t[] frame at syscall_frame_ptr.
// Layout: [EAX, ECX, EDX, EBX, EBP, ESI, EDI, user_EIP, CS, EFLAGS, user_ESP, SS]
#define FRAME_EAX 0
#define FRAME_ECX 1
#define FRAME_EDX 2
#define FRAME_EBX 3
#define FRAME_EBP 4
#define FRAME_ESI 5
#define FRAME_EDI 6
#define FRAME_EIP 7
#define FRAME_CS 8
#define FRAME_EFLAGS 9
#define FRAME_ESP 10
#define FRAME_SS 11

static const sig_default_action_t default_actions[32] = {
    [0] = SIG_ACTION_IGNORE,
    [SIGHUP] = SIG_ACTION_TERMINATE,
    [SIGINT] = SIG_ACTION_TERMINATE,
    [SIGQUIT] = SIG_ACTION_TERMINATE,
    [SIGILL] = SIG_ACTION_TERMINATE,
    [SIGTRAP] = SIG_ACTION_TERMINATE,
    [SIGABRT] = SIG_ACTION_TERMINATE,
    [SIGBUS] = SIG_ACTION_TERMINATE,
    [SIGFPE] = SIG_ACTION_TERMINATE,
    [SIGKILL] = SIG_ACTION_TERMINATE,
    [SIGUSR1] = SIG_ACTION_TERMINATE,
    [SIGSEGV] = SIG_ACTION_TERMINATE,
    [SIGUSR2] = SIG_ACTION_TERMINATE,
    [SIGPIPE] = SIG_ACTION_TERMINATE,
    [SIGALRM] = SIG_ACTION_TERMINATE,
    [SIGTERM] = SIG_ACTION_TERMINATE,
    [SIGSTKFLT] = SIG_ACTION_TERMINATE,
    [SIGCHLD] = SIG_ACTION_IGNORE,
    [SIGCONT] = SIG_ACTION_IGNORE,
    [SIGSTOP] = SIG_ACTION_TERMINATE,
    [SIGTSTP] = SIG_ACTION_TERMINATE,
    [SIGTTIN] = SIG_ACTION_TERMINATE,
    [SIGTTOU] = SIG_ACTION_TERMINATE,
    [SIGURG] = SIG_ACTION_IGNORE,
    [SIGXCPU] = SIG_ACTION_TERMINATE,
    [SIGXFSZ] = SIG_ACTION_TERMINATE,
    [SIGVTALRM] = SIG_ACTION_TERMINATE,
    [SIGPROF] = SIG_ACTION_TERMINATE,
    [SIGWINCH] = SIG_ACTION_IGNORE,
    [SIGIO] = SIG_ACTION_TERMINATE,
    [SIGPWR] = SIG_ACTION_TERMINATE,
    [SIGSYS] = SIG_ACTION_TERMINATE,
};

void task_reset_sighandler(task_t* task, int signum)
{
    if (task == NULL || signum < 0 || signum > 31) {
        return;
    }

    task->sig_handlers[signum].sa_flags = 0;
    task->sig_handlers[signum].sa_handler = SIG_DFL;
}

void task_signal_dispatch_pending(void)
{
    task_t* task = scheduler_get_current_task();
    if (task == NULL) {
        return;
    }

    sigset_t deliverable = task->sig_pending & ~task->sig_blocked;
    if (deliverable == 0) {
        return;
    }

    int sig = __builtin_ctz(deliverable);
    sigdelset(&task->sig_pending, sig);

    struct sigaction* sa = &task->sig_handlers[sig];

    if (sa->sa_handler == SIG_IGN) {
        return;
    }

    if (sa->sa_handler == SIG_DFL) {
        if (default_actions[sig] == SIG_ACTION_TERMINATE) {
            task_exit(-sig);
        }
        return;
    }

    uint32_t* frame = (uint32_t*)task->context.syscall_frame_ptr;
    uint32_t user_esp = frame[FRAME_ESP];

    uint32_t new_user_esp = (user_esp - (uint32_t)sizeof(sigframe_t)) & ~0xFU;

    if (
        new_user_esp < VMM_USER_START
        || new_user_esp + (uint32_t)sizeof(sigframe_t) > VMM_USER_END
    ) {
        task_exit(-SIGSEGV);
    }

    sigframe_t* sf = (sigframe_t*)new_user_esp;

    // Inline trampoline: mov eax, SYS_SIGRETURN; int 0x80; nop
    sf->trampoline[0] = 0xb8;
    sf->trampoline[1] = SYS_SIGRETURN;
    sf->trampoline[2] = 0x00;
    sf->trampoline[3] = 0x00;
    sf->trampoline[4] = 0x00;
    sf->trampoline[5] = 0xcd;
    sf->trampoline[6] = 0x80;
    sf->trampoline[7] = 0x90;

    sf->pretcode = new_user_esp + (uint32_t)offsetof(sigframe_t, trampoline);
    sf->sig = sig;

    sf->ctx.eax = frame[FRAME_EAX];
    sf->ctx.ecx = frame[FRAME_ECX];
    sf->ctx.edx = frame[FRAME_EDX];
    sf->ctx.ebx = frame[FRAME_EBX];
    sf->ctx.ebp = frame[FRAME_EBP];
    sf->ctx.esi = frame[FRAME_ESI];
    sf->ctx.edi = frame[FRAME_EDI];
    sf->ctx.eip = frame[FRAME_EIP];
    sf->ctx.cs = frame[FRAME_CS];
    sf->ctx.eflags = frame[FRAME_EFLAGS];
    sf->ctx.esp = frame[FRAME_ESP];
    sf->ctx.ss = frame[FRAME_SS];
    sf->ctx.old_sig_blocked = task->sig_blocked;

    if (!(sa->sa_flags & SA_NODEFER)) {
        sigaddset(&task->sig_blocked, sig);
    }
    task->sig_blocked |= sa->sa_mask;

    if (sa->sa_flags & SA_RESETHAND) {
        task_reset_sighandler(task, sig);
    }

    frame[FRAME_EIP] = (uint32_t)sa->sa_handler;
    frame[FRAME_ESP] = new_user_esp;
}
