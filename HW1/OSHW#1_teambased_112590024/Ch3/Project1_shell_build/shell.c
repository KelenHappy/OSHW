#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80
#define MAX_HISTORY 100
#define HISTORY_FILE ".shell_history"

char history[MAX_HISTORY][MAX_LINE];
int history_count = 0;

void load_history() {
    FILE *fp = fopen(HISTORY_FILE, "r");
    if (!fp) return;
    
    while (fgets(history[history_count], MAX_LINE, fp) && history_count < MAX_HISTORY) {
        history[history_count][strcspn(history[history_count], "\n")] = 0;
        history_count++;
    }
    fclose(fp);
}

void save_history() {
    FILE *fp = fopen(HISTORY_FILE, "w");
    if (!fp) return;
    
    for (int i = 0; i < history_count; i++) {
        fprintf(fp, "%s\n", history[i]);
    }
    fclose(fp);
}

void add_history(char *cmd) {
    if (history_count < MAX_HISTORY) {
        strcpy(history[history_count++], cmd);
    } else {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        strcpy(history[MAX_HISTORY - 1], cmd);
    }
    save_history();
}

void print_history() {
    printf("\n========== Command History ==========\n");
    for (int i = 0; i < history_count; i++) {
        printf("%3d: %s\n", i + 1, history[i]);
    }
    printf("=====================================\n\n");
}

int parse_redirect(char **args, char *in_file, char *out_file, int *append) {
    int pipe_pos = -1;
    in_file[0] = 0;
    out_file[0] = 0;
    *append = 0;

    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], "<") == 0) {
            if (args[i + 1]) {
                strcpy(in_file, args[i + 1]);
                args[i] = NULL;
            }
        } else if (strcmp(args[i], ">>") == 0) {
            if (args[i + 1]) {
                strcpy(out_file, args[i + 1]);
                *append = 1;
                args[i] = NULL;
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (args[i + 1]) {
                strcpy(out_file, args[i + 1]);
                *append = 0;
                args[i] = NULL;
            }
        } else if (strcmp(args[i], "|") == 0) {
            pipe_pos = i;
            args[i] = NULL;
            break;
        }
    }
    return pipe_pos;
}

void execute_cmd(char **args, char *in_file, char *out_file, int append) {
    if (in_file[0]) {
        int fd = open(in_file, O_RDONLY);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    
    if (out_file[0]) {
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        int fd = open(out_file, flags, 0644);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    
    execvp(args[0], args);
    perror("execvp");
    exit(1);
}

void execute_pipe(char **cmd1, char **cmd2, char *in_file, char *out_file, int append) {
    int pfd[2];
    pid_t pid1, pid2;

    if (pipe(pfd) < 0) {
        perror("pipe");
        return;
    }

    pid1 = fork();
    if (pid1 == 0) {
        close(pfd[0]);
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]);
        
        if (in_file[0]) {
            int fd = open(in_file, O_RDONLY);
            if (fd >= 0) {
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
        }
        
        execvp(cmd1[0], cmd1);
        perror("execvp");
        exit(1);
    }

    pid2 = fork();
    if (pid2 == 0) {
        close(pfd[1]);
        dup2(pfd[0], STDIN_FILENO);
        close(pfd[0]);
        
        if (out_file[0]) {
            int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
            int fd = open(out_file, flags, 0644);
            if (fd >= 0) {
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
        }
        
        execvp(cmd2[0], cmd2);
        perror("execvp");
        exit(1);
    }

    close(pfd[0]);
    close(pfd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

int main() {
    char line[MAX_LINE];
    char line_copy[MAX_LINE];
    char *args[MAX_LINE / 2 + 1];
    char in_file[MAX_LINE];
    char out_file[MAX_LINE];
    int append_mode, pipe_pos;
    pid_t pid;

    load_history();
    
    printf("========================================\n");
    printf("=== Welcome to Simple UNIX Shell ===\n");
    printf("========================================\n");
    printf("Commands: exit (quit), history (show history), cd (change directory)\n");
    printf("Features: input/output redirection (<, >, >>), pipes (|)\n\n");

    while (1) {
        printf("osh> ");
        fflush(stdout);

        if (!fgets(line, MAX_LINE, stdin)) {
            break;
        }

        line[strcspn(line, "\n")] = 0;

        if (!line[0]) {
            continue;
        }

        /* Handle exit command */
        if (strcmp(line, "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }

        /* Handle history command */
        if (strcmp(line, "history") == 0) {
            print_history();
            continue;
        }

        /* Handle cd command */
        if (strncmp(line, "cd ", 3) == 0) {
            if (chdir(line + 3) != 0) {
                perror("cd");
            }
            add_history(line);
            continue;
        }

        add_history(line);

        /* Parse command line */
        strcpy(line_copy, line);
        int argc = 0;
        char *ptr = strtok(line_copy, " ");
        while (ptr && argc < MAX_LINE / 2) {
            args[argc++] = ptr;
            ptr = strtok(NULL, " ");
        }
        args[argc] = NULL;

        if (!args[0]) {
            continue;
        }

        /* Check for redirection and pipes */
        pipe_pos = parse_redirect(args, in_file, out_file, &append_mode);

        /* Fork and execute */
        pid = fork();
        if (pid == 0) {
            if (pipe_pos >= 0) {
                /* Handle pipe */
                char *cmd2[MAX_LINE / 2 + 1];
                int j = 0;
                for (int i = pipe_pos + 1; args[i]; i++) {
                    cmd2[j++] = args[i];
                }
                cmd2[j] = NULL;
                args[pipe_pos] = NULL;
                
                execute_pipe(args, cmd2, in_file, out_file, append_mode);
                exit(0);
            } else {
                /* Handle regular command with redirection */
                execute_cmd(args, in_file, out_file, append_mode);
                exit(0);
            }
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }

    return 0;
}