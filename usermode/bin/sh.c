#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

#define COMM_BUF_SIZE 256
#define READ_BUF_SIZE 8
#define STDIN_FD 0
#define CURSOR_ENABLE "\033[25h"
#define CURSOR_DISABLE "\033[25l"
#define HISTORY_MAXENTRIES 64

#define add_char(c) do {\
    words[curr_word] = realloc(words[curr_word], curr_word_len + 2); \
    words[curr_word][curr_word_len] = c; \
    words[curr_word][curr_word_len + 1] = '\0'; \
    curr_word_len++; } while (0);

#define start_new_line do {\
    words = realloc(words, sizeof(char*) * (curr_word + 3)); \
    words[curr_word + 1] = NULL; \
    words[curr_word + 2] = NULL; \
    curr_word++; \
    curr_word_len = 0; } while (0);

typedef struct {
    char* comm_name;
    char** argv;
    char** envp_override;
    char* input;
    char* output;
    bool output_append;
} command_t;

static char comm_buffer[COMM_BUF_SIZE] = {0};
static int comm_cursor_loc = 0;
static int comm_buffer_len = 0;
static int last_comm_status = 0;

static char* history[HISTORY_MAXENTRIES] = {0};
static int history_entries_count = 0;

static struct termios default_termios;
static struct termios sh_termios;

static command_t** parse_command(int* command_count)
{
    if (comm_buffer_len == 0) {
        return NULL;
    }
    // NULL-terminated list of words separated by space
    char** words = malloc(sizeof(char*) * 2);
    words[0] = NULL;
    words[1] = NULL;
    int curr_word = 0; // words count = curr_word + 2
    int curr_word_len = 0;

    char in_quote = '\0';
    bool prev_backslash = false;
    for (int i = 0; i < comm_buffer_len; i++) {
        char c = comm_buffer[i];
        if (c == '\\' && !prev_backslash) {
            prev_backslash = true;
            continue;
        }
        else if (prev_backslash) {
            // TODO: parsing different characters? Like new line for example
            add_char(c);
            prev_backslash = false;
            continue;
        }

        if ((c == '"' || c == '\'') && in_quote == '\0')
        {
            // Start in_qoute block
            in_quote = c;
            if (curr_word_len > 0) {
                start_new_line;
            }
            // Initialize an empty word so it's not NULL even if quote is empty
            if (words[curr_word] == NULL) {
                words[curr_word] = malloc(1);
                words[curr_word][0] = '\0';
            }
        }
        else if ((c == '"' || c == '\'') && in_quote == c)
        {
            // End in_qoute block
            in_quote = '\0';
            start_new_line;
        }
        else if ((c == ' ' || c == '#') && in_quote != '\0') {
            // Space or #, but inside quote - add it
            add_char(c);
        }
        else if (c == ' ' && curr_word_len == 0) {
            // Starting with space - ignore
            continue;
        }
        else if (c == ' ') {
            // Need to start a new word
            start_new_line;
        }
        else if (c == '#') {
            // Comment started - end of parsing
            break;
        }
        else if (c == '|' && in_quote == '\0') {
            // Unescaped pipe outside quotes -> standalone token
            if (curr_word_len > 0) {
                start_new_line;
            }
            if (words[curr_word] == NULL) {
                words[curr_word] = malloc(1);
                words[curr_word][0] = '\0';
            }
            add_char('|');
            start_new_line;
        }
        else if ((c == '>' || c == '<') && in_quote == '\0') {
            // Redirecting of the stdin/out to given path
            if (curr_word_len > 0) {
                start_new_line;
            }
            if (words[curr_word] == NULL) {
                words[curr_word] = malloc(1);
                words[curr_word][0] = '\0';
            }
            add_char(c);
            // Check if appending mode:
            if (
                c == '>'
                && (i + 1) < comm_buffer_len
                && comm_buffer[i + 1] == '>'
            ) {
                add_char('>');
                i++;
            }
            start_new_line;
        }
        else {
            // Just append character
            add_char(c);
        }
    }
    int words_count = curr_word;
    if (words[curr_word] == NULL) {
        words_count--;
    }

    // NULL-terminated list of command objects
    command_t** commands = malloc(sizeof(command_t*) * 2);
    commands[0] = malloc(sizeof(command_t));
    commands[0]->comm_name = NULL;
    commands[0]->argv = malloc(sizeof(char*));
    commands[0]->argv[0] = NULL;
    commands[0]->envp_override = malloc(sizeof(char*));
    commands[0]->envp_override[0] = NULL;
    commands[0]->input = NULL;
    commands[0]->output = NULL;
    commands[0]->output_append = false;
    commands[1] = NULL;
    int curr_command_idx = 0;
    int argv_count = 0;
    int envp_count = 0;
    bool accepting_envs = true;
    *command_count = 0;

    for (int i = 0; i <= words_count; i++) {
        char* word = words[i];
        if (strcmp(word, ">") == 0 || strcmp(word, ">>") == 0) {
            commands[curr_command_idx]->output_append = strcmp(word, ">>") == 0;
            i++;
            if (i <= words_count) {
                commands[curr_command_idx]->output = strdup(words[i]);
            }
            continue;
        }
        if (strcmp(word, "<") == 0) {
            i++;
            if (i <= words_count) {
                commands[curr_command_idx]->input = strdup(words[i]);
            }
            continue;
        }
        if (strcmp(word, "|") == 0) {
            curr_command_idx++;
            argv_count = 0;
            envp_count = 0;
            accepting_envs = true;
            commands = realloc(
                commands,
                sizeof(command_t*) * (curr_command_idx + 2)
            );
            commands[curr_command_idx] = malloc(sizeof(command_t));
            commands[curr_command_idx]->comm_name = NULL;
            commands[curr_command_idx]->argv = malloc(sizeof(char*));
            commands[curr_command_idx]->argv[0] = NULL;
            commands[curr_command_idx]->envp_override = malloc(sizeof(char*));
            commands[curr_command_idx]->envp_override[0] = NULL;
            commands[curr_command_idx]->input = NULL;
            commands[curr_command_idx]->output = NULL;
            commands[curr_command_idx]->output_append = false;
            commands[curr_command_idx + 1] = NULL;
            continue;
        }

        command_t* cmd = commands[curr_command_idx];
        if (accepting_envs && strchr(word, '=') != NULL) {
            cmd->envp_override = realloc(
                cmd->envp_override,
                sizeof(char*) * (envp_count + 2)
            );
            cmd->envp_override[envp_count] = malloc(strlen(word) + 1);
            strcpy(cmd->envp_override[envp_count], word);
            cmd->envp_override[envp_count + 1] = NULL;
            envp_count++;
        } else {
            accepting_envs = false;
            if (cmd->comm_name == NULL) {
                cmd->comm_name = malloc(strlen(word) + 1);
                strcpy(cmd->comm_name, word);
                (*command_count)++;
            }
            cmd->argv = realloc(cmd->argv, sizeof(char*) * (argv_count + 2));
            cmd->argv[argv_count] = malloc(strlen(word) + 1);
            strcpy(cmd->argv[argv_count], word);
            cmd->argv[argv_count + 1] = NULL;
            argv_count++;
        }
    }

    // Cleanup
    for (int i = 0; i <= words_count; i++) {
        free(words[i]);
    }
    free(words);

    return commands;
}

static void add_curr_buffer_to_history(void)
{
    if (history_entries_count == HISTORY_MAXENTRIES) {
        history_entries_count--;
    }

    // Shift entries
    for (size_t i = history_entries_count; i > 0; i--) {
        history[i] = history[i - 1];
    }
    // Put newest entry at the start
    history[0] = strdup(comm_buffer);
    history_entries_count++;
}

static bool handle_command(void)
{
    printf("\n");
    if (!strcmp(comm_buffer, "exit")) {
        return false;
    }
    int commands_count = 0;
    command_t** commands = parse_command(&commands_count);
    if (commands_count == 0) {
        return true;
    }

    add_curr_buffer_to_history();

    int pipeline_len = 0;
    while (commands[pipeline_len] != NULL) pipeline_len++;

    int (*pipes)[2] = NULL;
    if (pipeline_len > 1) {
        pipes = malloc(sizeof(int[2]) * (pipeline_len - 1));
        for (int i = 0; i < pipeline_len - 1; i++) {
            pipe(pipes[i]);
        }
    }

    int* pids = NULL;
    int pids_count = 0;
    bool parent = true;
    pid_t pipeline_pgid = 0;
    for (int cmd_index = 0; cmd_index < pipeline_len; cmd_index++) {
        command_t* cmd = commands[cmd_index];
        if (cmd->comm_name == NULL) continue;
        if (!strcmp(cmd->comm_name, "cd")) {
            int res;
            errno = 0;
            if (cmd->argv[0] == NULL || cmd->argv[1] == NULL) {
                res = chdir("/"); // TODO: should go to the home directory
            } else {
                res = chdir(cmd->argv[1]);
            }
            if (res == -1) {
                last_comm_status = errno;
            }
            continue;
        }
        pids = realloc(pids, sizeof(int) * (pids_count + 1));
        int new_pid = fork();
        if (new_pid < 0) {
            puts("ERROR!\n");
        }
        else if (new_pid > 0) {
            if (pipeline_pgid == 0) pipeline_pgid = new_pid;
            setpgid(new_pid, pipeline_pgid);
            pids[pids_count] = new_pid;
            pids_count++;
        }
        else {
            parent = false;
            // pipeline_pgid is 0 for the first child — setpgid(0,0) makes
            // the child its own group leader
            setpgid(0, pipeline_pgid);
            if (cmd->comm_name == NULL) {
                exit(0);
            }
            if (pipes != NULL) {
                if (cmd_index > 0) {
                    dup2(pipes[cmd_index - 1][0], STDIN_FILENO);
                }
                if (cmd_index < pipeline_len - 1) {
                    dup2(pipes[cmd_index][1], STDOUT_FILENO);
                }
                for (int i = 0; i < pipeline_len - 1; i++) {
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }
            }
            if (cmd->output != NULL) {
                int flags = O_WRONLY | O_CREAT
                    | (cmd->output_append ? O_APPEND : O_TRUNC);
                int fd = open(cmd->output, flags, 0644);
                if (fd >= 0) {
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
            }
            if (cmd->input != NULL) {
                int fd = open(cmd->input, O_RDONLY);
                if (fd >= 0) {
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
            }
            for (int j = 0; cmd->envp_override[j] != NULL; j++) {
                putenv(cmd->envp_override[j]);
            }
            execvp(cmd->comm_name, (const char**)cmd->argv);
            printf("sh: %s: command not found\n", cmd->comm_name);
            exit(127);
        }
    }

    if (pipes != NULL) {
        for (int i = 0; i < pipeline_len - 1; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
        free(pipes);
    }

    if (parent) {
        if (pipeline_pgid > 0) {
            tcsetpgrp(STDIN_FD, pipeline_pgid);
        }
        for (int i = 0; i < pids_count; i++) {
            waitpid(pids[i], &last_comm_status, 0);
        }
        tcsetpgrp(STDIN_FD, getpgid(0));
    }

    free(pids);
    for (int i = 0; i < pipeline_len; i++) {
        command_t* cmd = commands[i];
        free(cmd->comm_name);
        for (int j = 0; cmd->argv[j] != NULL; j++) {
            free(cmd->argv[j]);
        }
        free(cmd->argv);
        for (int j = 0; cmd->envp_override[j] != NULL; j++) {
            free(cmd->envp_override[j]);
        }
        free(cmd->envp_override);
        free(cmd->output);
        free(cmd->input);
        free(cmd);
    }
    free(commands);
    return true;
}

static bool escseq_check(char* buf, int count, char* test)
{
    int testsize = strlen(test);
    if (testsize > (count - 1)) {
        return false;
    }
    for (int i = 0; i < testsize; i++) {
        if (buf[i + 1] != test[i]) {
            return false;
        }
    }
    return true;
}

static void print_prompt(void)
{
    char cwd[128] = {0};
    getcwd(cwd, 127);
    time_t timestamp = time(NULL);
    struct tm* local = localtime(&timestamp);
    printf("\n\033[0m%02u:%02u:%02u \033[96m%s \033[%um%u\n\033[97m$\033[0m %s",
        local->tm_hour,
        local->tm_min,
        local->tm_sec,
        cwd,
        last_comm_status == 0 ? 92 : 91,
        last_comm_status,
        CURSOR_ENABLE
    );
}

static void load_history_entry(int entry)
{
    printf(CURSOR_DISABLE);
    // Navigate to the position, clear the rest of the line
    printf("\033[3G\033[K");
    if (entry >= 0 && entry < history_entries_count) {
        strcpy(comm_buffer, history[entry]);
        comm_buffer_len = strlen(comm_buffer);
        comm_cursor_loc = comm_buffer_len;
        printf(comm_buffer);
    } else {
        strcpy(comm_buffer, "");
        comm_buffer_len = 0;
        comm_cursor_loc = 0;
    }
    printf(CURSOR_ENABLE);
}

int main(void)
{
    // Setup input to disable canon mode
    tcgetattr(STDIN_FD, &sh_termios);
    memcpy(&default_termios, &sh_termios, sizeof(struct termios));
    sh_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FD, TCSANOW, &sh_termios);

    // Ignore SIGINTs
    struct sigaction int_handler;
    int_handler.sa_handler = SIG_IGN;
    sigaction(SIGINT, &int_handler, NULL);

    bool continue_exec = true;
    // Main command loop
    while (continue_exec) {
        print_prompt();
        memset(comm_buffer, 0, COMM_BUF_SIZE);
        comm_cursor_loc = 0;
        comm_buffer_len = 0;

        // Reading loop
        bool continue_reading = true;
        // -1 - no entry selected.
        int curr_history_entry = -1;
        while (continue_reading) {
            char read_buff[READ_BUF_SIZE] = {0};
            int readcount = read(0, read_buff, READ_BUF_SIZE - 1);
            for (int i = 0; i < readcount; i++) {
                char c = read_buff[i];
                if (c == '\n') {
                    continue_reading = false;
                    i = readcount;
                }
                else if (c == '\033') {
                    if (escseq_check(read_buff + i, readcount, "[A")) {
                        // Arrow up
                        i += 2;
                        int expected_history_entry = curr_history_entry + 1;
                        if (expected_history_entry < history_entries_count) {
                            curr_history_entry = expected_history_entry;
                            load_history_entry(curr_history_entry);
                        }
                    }
                    else if (escseq_check(read_buff + i, readcount, "[B")) {
                        // Arrow down
                        i += 2;
                        int expected_history_entry = curr_history_entry - 1;
                        if (expected_history_entry < 0) {
                            curr_history_entry = -1;
                        } else {
                            curr_history_entry = expected_history_entry;
                        }
                        load_history_entry(curr_history_entry);
                    }
                    else if (escseq_check(read_buff + i, readcount, "[C")) {
                        // Arrow right
                        i += 2;
                        if (comm_cursor_loc < comm_buffer_len) {
                            comm_cursor_loc++;
                            printf("\033[C");
                        }
                    }
                    else if (escseq_check(read_buff + i, readcount, "[D")) {
                        // Arrow left
                        i += 2;
                        if (comm_cursor_loc > 0) {
                            comm_cursor_loc--;
                            printf("\033[D");
                        }
                    }
                    else if (escseq_check(read_buff + i, readcount, "[H")) {
                        // Home
                        i += 2;
                        comm_cursor_loc = 0;
                        printf("\033[3G");
                    }
                    else if (escseq_check(read_buff + i, readcount, "[F")) {
                        // End
                        i += 2;
                        comm_cursor_loc = comm_buffer_len;
                        printf("\033[%uG", comm_cursor_loc + 3);
                    }
                    else {
                        i = readcount;
                    }
                }
                else if (c == '\b') {
                    if (comm_cursor_loc > 0) {
                        for (int j = comm_cursor_loc - 1; j < comm_buffer_len - 1; j++) {
                            comm_buffer[j] = comm_buffer[j + 1];
                        }
                        comm_buffer[comm_buffer_len - 1] = '\0';
                        comm_cursor_loc--;
                        comm_buffer_len--;
                        printf("\033[D");
                        for (int j = comm_cursor_loc; j < comm_buffer_len; j++) {
                            putchar(comm_buffer[j]);
                        }
                        putchar(' ');
                        printf("\033[%dD", comm_buffer_len - comm_cursor_loc + 1);
                    }
                }
                else if (comm_cursor_loc < COMM_BUF_SIZE) {
                    if (comm_cursor_loc != comm_buffer_len) {
                        // Shift all characters in the buffer
                        for (int j = comm_buffer_len; j >= comm_cursor_loc; j--)
                        {
                            comm_buffer[j] = comm_buffer[j-1];
                        }
                        // Print all characters
                        putchar(c);
                        for (int j = comm_cursor_loc + 1; j <= comm_buffer_len; j++)
                        {
                            putchar(comm_buffer[j]);
                        }
                        // Return to correct cursor position
                        printf("\033[%uD", comm_buffer_len - comm_cursor_loc);
                    } else {
                        putchar(c);
                    }
                    comm_buffer[comm_cursor_loc] = c;
                    comm_cursor_loc++;
                    comm_buffer_len++;
                }
            }
        }

        tcsetattr(STDIN_FD, TCSANOW, &default_termios);
        continue_exec = handle_command();
        tcsetattr(STDIN_FD, TCSANOW, &sh_termios);
    }

    // Recover original termios settings
    tcsetattr(STDIN_FD, TCSANOW, &default_termios);
    return last_comm_status;
}
