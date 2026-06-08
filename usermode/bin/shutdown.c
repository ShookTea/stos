#include <sys/reboot.h>
#include <stddef.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

#define OPT_SHORT "Pr"

static struct option opts[] = {
    { "help", no_argument, NULL, 0 },
    { "poweroff", no_argument, NULL, 'P' },
    { "reboot", no_argument, NULL, 'r' },
    { NULL, 0, NULL, 0 },
};

static void print_usage(void)
{
    puts("Usage:");
    puts("  shutdown [options]");
    puts("");
    puts("META OPTIONS");
    puts("  --help                  Shows usage info");
    puts("");
    puts("SHUTDOWN OPTIONS");
    puts("  -P, --poweroff          Shuts down the computer (default action)");
    puts("  -r, --reboot            Reboots the computer");
}

int main(int argc, char** argv)
{
    int op = RB_POWER_OFF;
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
            case 'P': op = RB_POWER_OFF; break;
            case 'r': op = RB_AUTOBOOT; break;
            default: break;
        }
    }

    reboot(op);
    // `reboot(op)` should never return.
    return 1;
}
