#include "signal.h"
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>

// Original termios value, for later recovery
static struct termios orig_termios;
static int term_width = 0;
static int term_height = 0;

static char* filename = "tmp";

static bool textedit_init(void)
{
    // Load termios config and store backup
    struct termios termios;
    tcgetattr(STDIN_FILENO, &termios);
    memcpy(&orig_termios, &termios, sizeof(struct termios));

    // Set flags
    termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &termios);

    // Clear terminal, and disable autowrapping
    printf("\033[2J\033[7l");
    // Measure screen size

    printf("\033[9999;9999H\033[6n");
    char buf[32] = {0};
    int read_count = read(STDIN_FILENO, buf, 31);
    if (read_count > 0 && buf[0] == '\033' && buf[1] == '[') {
        char* ptr = buf + 2; // skip start
        ptr[strlen(ptr) - 1] = '\0';
        term_height = atoi(strtok(ptr, ";"));
        term_width = atoi(strtok(NULL, ";"));
    } else {
        return false;
    }

    // Print border
    printf("\033[H┌──");
    int printed = 3;
    printed += printf(" %s ", filename);
    for (int i = printed; i < term_width - 1; i++) {
        printf("─");
    }
    printf("┐");
    for (int i = 1; i < term_height - 1; i++) {
        printf("\033[%u;1H│", i + 1);
        for (int j = 1; j < term_width - 1; j++) {
            printf(" ");
        }
        printf("│");
    }
    printf("\033[%u;1H└", term_height);
    for (int i = 1; i < term_width - 1; i++) {
        printf("─");
    }
    printf("┘");

    return true;
}

static void textedit_close(void)
{
    // Clean screen, reset cursor position, reenable autowrapping
    printf("\033[2J\033[H\033[7h");
    // Restore original termios value
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

static bool sigint_received = false;

static void sigint_handler(int signum __attribute__((unused)))
{
    sigint_received = true;
}

int main(void)
{
    if (!textedit_init()) {
        return 1;
    }

    struct sigaction act;
    act.sa_flags = 0;
    act.sa_mask = 0;
    act.sa_handler = sigint_handler;
    sigaction(SIGINT, &act, NULL);

    while (!sigint_received) {
        sched_yield();
    }

    textedit_close();
    return 0;
}
