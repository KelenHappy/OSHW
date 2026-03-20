/*
 * Exercise 3.21 - Collatz conjecture using POSIX shared memory
 * Build (Linux):
 *   gcc -std=c11 -Wall -Wextra -O2 -o Collatz collatz_fork.c -lrt
 * Run:
 *   ./Collatz 35
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <positive-integer>\n", prog);
}

static unsigned long long parse_positive_ull(const char *s) {
    char *end = NULL;
    errno = 0;

    if (s == NULL || *s == '\0') {
        fprintf(stderr, "Error: missing number\n");
        exit(EXIT_FAILURE);
    }

    if (s[0] == '-') {
        fprintf(stderr, "Error: number must be a positive integer (> 0); negative values are not allowed\n");
        exit(EXIT_FAILURE);
    }
    if (s[0] == '+') {
        s++;
        if (*s == '\0') {
            fprintf(stderr, "Error: not a valid integer: +\n");
            exit(EXIT_FAILURE);
        }
    }

    for (const char *p = s; *p; ++p) {
        if (!isdigit((unsigned char)*p)) {
            fprintf(stderr, "Error: not a valid integer: %s\n", s);
            exit(EXIT_FAILURE);
        }
    }

    unsigned long long v = strtoull(s, &end, 10);

    if (errno == ERANGE) {
        fprintf(stderr, "Error: number out of range: %s\n", s);
        exit(EXIT_FAILURE);
    }
    if (end == s || *end != '\0') {
        fprintf(stderr, "Error: not a valid integer: %s\n", s);
        exit(EXIT_FAILURE);
    }
    if (v == 0) {
        fprintf(stderr, "Error: number must be a positive integer (> 0)\n");
        exit(EXIT_FAILURE);
    }
    return v;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    unsigned long long n = parse_positive_ull(argv[1]);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        // Child: generate and output sequence
        while (1) {
            printf("%llu", n);
            if (n == 1) {
                putchar('\n');
                fflush(stdout);
                _exit(EXIT_SUCCESS);
            }
            printf(", ");

            if (n % 2ULL == 0ULL) {
                n = n / 2ULL;
            } else {
                // Beware overflow for very large n; assignment assumes typical homework inputs.
                n = 3ULL * n + 1ULL;
            }
        }
    } else {
        // Parent: wait for child to finish
        int status = 0;
        pid_t w = waitpid(pid, &status, 0);
        if (w < 0) {
            perror("waitpid");
            return EXIT_FAILURE;
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return EXIT_FAILURE;
    }
}
