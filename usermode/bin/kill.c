#include "unistd.h"
#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPT_SHORT "s:"

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
    { "signal", required_argument, NULL, 's' },
    { NULL, 0, NULL, 0 },
};

static void print_usage(void) {
    // TODO
}

typedef struct {
    int value;
    const char* name;
} sigtype_t;

// List of all available signals, terminated with value=0
static sigtype_t sigtypes[] = {
    { SIGHUP, "HUP" }, { SIGINT, "INT" }, { SIGQUIT, "QUIT" },
    { SIGILL, "ILL" }, { SIGTRAP, "TRAP" }, { SIGABRT, "ABRT" },
    { SIGBUS, "BUS" }, { SIGFPE, "FPE" }, { SIGKILL, "KILL" },
    { SIGUSR1, "USR1" }, { SIGSEGV, "SEGV" }, { SIGUSR2, "USR2" },
    { SIGPIPE, "PIPE" }, { SIGALRM, "ALRM" }, { SIGTERM, "TERM" },
    { SIGSTKFLT, "STKFLT" }, { SIGCHLD, "CHLD" }, { SIGCONT, "CONT" },
    { SIGSTOP, "STOP" }, { SIGTSTP, "TSTP" }, { SIGTTIN, "TTIN" },
    { SIGTTOU, "TTOU" }, { SIGURG, "URG" }, { SIGXCPU, "XCPU" },
    { SIGXFSZ, "XFSZ" }, { SIGVTALRM, "VTALRM" }, { SIGPROF, "PROF" },
    { SIGWINCH, "WINCH" }, { SIGIO, "IO" }, { SIGPWR, "PWR" },
    { SIGSYS, "SYS" }, { 0, "" },
};


static int signal_to_send = SIGTERM;

int main(int argc, char** argv)
{
    int c;
    while (true) {
        int option_index = 0;
        c = getopt_long(argc, argv, OPT_SHORT, opts, &option_index);
        if (c == -1) break;
        switch (c) {
            case 0: {
                if (!strcmp(opts[option_index].name, "help")) {
                    print_usage();
                    return 0;
                }
                break;
            }
            case 's': {
                int newsig = -1;
                if (isdigit(optarg[0])) {
                    newsig = atoi(optarg);
                }
                else {
                    char* to_read = optarg;
                    if (
                        optarg[0] == 'S' && optarg[1] == 'I' && optarg[2] == 'G'
                    ) {
                        to_read += 3;
                    }
                    sigtype_t* t = sigtypes;
                    while (t->value != 0) {
                        if (!strcmp(t->name, to_read)) {
                            newsig = t->value;
                            break;
                        }
                        t++;
                    }
                }

                if (newsig <= 0 || newsig > 31) {
                    dprintf(
                        STDERR_FILENO,
                        "kill: option --signal doesn't support value '%s'\n",
                        optarg
                    );
                    return 1;
                } else {
                    signal_to_send = newsig;
                }
                break;
            }
            default: break;
        }
    }

    return 0;
}
