#define _POSIX_C_SOURCE 200809L

/*
 * Exercise 3.22 - Collatz conjecture using POSIX shared memory
 * Build (Linux):
 *   gcc -std=c11 -Wall -Wextra -O2 -o CollatzSHM collatz_shm.c -lrt
 * Run:
 *   ./CollatzSHM 35
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHM_SIZE (64 * 1024) /* 64 KiB */

static void die_errno(const char *what) {
    int e = errno;
    fprintf(stderr, "Error: %s: %s (errno=%d)\n", what, strerror(e), e);
    exit(EXIT_FAILURE);
}

static void die_msg(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(EXIT_FAILURE);
}

/*
 * Parse a *positive* integer from argv.
 * - Rejects negatives explicitly (e.g., "-5")
 * - Rejects 0
 * - Rejects non-digits
 * Returns parsed value as unsigned long long.
 */
static unsigned long long parse_positive_ull_strict(const char *s) {
    if (s == NULL || *s == '\0') {
        die_msg("missing positive integer argument");
    }

    if (s[0] == '-') {
        die_msg("n must be a positive integer (> 0); negative values are not allowed");
    }
    if (s[0] == '+') {
        /* allow leading '+' but require at least one digit after it */
        s++;
        if (*s == '\0') die_msg("argument is not a valid integer");
    }

    for (const char *p = s; *p; ++p) {
        if (!isdigit((unsigned char)*p)) {
            die_msg("argument is not a valid positive integer (digits only)");
        }
    }

    errno = 0;
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);

    if (errno == ERANGE) die_msg("number out of range");
    if (end == s || *end != '\0') die_msg("argument is not a valid integer");
    if (v == 0ULL) die_msg("n must be a positive integer (> 0)");

    return v;
}

static void shm_append_bytes(char *base, size_t cap, size_t *len_io, const char *s, size_t n) {
    if (*len_io > cap) die_msg("internal: buffer length overflow");
    if (n > cap - *len_io - 1) {
        die_msg("shared memory buffer too small for Collatz sequence");
    }
    memcpy(base + *len_io, s, n);
    *len_io += n;
    base[*len_io] = '\0';
}

static void shm_append_ull(char *base, size_t cap, size_t *len_io, unsigned long long v) {
    if (*len_io >= cap) die_msg("internal: buffer length overflow");

    int needed = snprintf(base + *len_io, cap - *len_io, "%llu", v);
    if (needed < 0) die_msg("snprintf failed");
    if ((size_t)needed >= cap - *len_io) {
        die_msg("shared memory buffer too small for Collatz sequence");
    }
    *len_io += (size_t)needed;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <positive-integer>\n", argv[0]);
        return EXIT_FAILURE;
    }

    unsigned long long n = parse_positive_ull_strict(argv[1]);

    /* Make shm name unique per run to avoid collisions. Must start with '/'. */
    char shm_name[64];
    pid_t parent_pid = getpid();
    if (snprintf(shm_name, sizeof(shm_name), "/collatz_shm_%ld", (long)parent_pid) >= (int)sizeof(shm_name)) {
        die_msg("internal: shm name too long");
    }

    /* a) Establish shared-memory object */
    int shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shm_fd < 0) {
        die_errno("shm_open");
    }

    if (ftruncate(shm_fd, SHM_SIZE) != 0) {
        int saved = errno;
        (void)close(shm_fd);
        (void)shm_unlink(shm_name);
        errno = saved;
        die_errno("ftruncate");
    }

    void *addr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (addr == MAP_FAILED) {
        int saved = errno;
        (void)close(shm_fd);
        (void)shm_unlink(shm_name);
        errno = saved;
        die_errno("mmap");
    }

    /* Mapping persists after closing fd. */
    if (close(shm_fd) != 0) {
        int saved = errno;
        (void)munmap(addr, SHM_SIZE);
        (void)shm_unlink(shm_name);
        errno = saved;
        die_errno("close(shm_fd)");
    }

    char *shm_buf = (char *)addr;
    shm_buf[0] = '\0';

    /* b) fork child and wait */
    pid_t pid = fork();
    if (pid < 0) {
        int saved = errno;
        (void)munmap(addr, SHM_SIZE);
        (void)shm_unlink(shm_name);
        errno = saved;
        die_errno("fork");
    }

    if (pid == 0) {
        /* Child: write sequence into shared memory */
        size_t len = 0;

        while (1) {
            shm_append_ull(shm_buf, SHM_SIZE, &len, n);

            if (n == 1ULL) {
                shm_append_bytes(shm_buf, SHM_SIZE, &len, "\n", 1);

                /* Best-effort: flush mapping */
                (void)msync(addr, SHM_SIZE, MS_SYNC);
                (void)munmap(addr, SHM_SIZE);
                _exit(EXIT_SUCCESS);
            }

            shm_append_bytes(shm_buf, SHM_SIZE, &len, ", ", 2);

            if ((n % 2ULL) == 0ULL) {
                n = n / 2ULL;
            } else {
                /* Potential overflow for very large n; acceptable for typical homework inputs. */
                n = 3ULL * n + 1ULL;
            }
        }
    }

    /* Parent: wait for child to finish (synchronization) */
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        int saved = errno;
        (void)munmap(addr, SHM_SIZE);
        (void)shm_unlink(shm_name);
        errno = saved;
        die_errno("waitpid");
    }

    /* c) Output shared memory contents */
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        fputs(shm_buf, stdout);
        fflush(stdout);
    } else {
        fprintf(stderr, "Child did not exit cleanly.\n");
        if (shm_buf[0] != '\0') {
            fprintf(stderr, "Partial output:\n%s", shm_buf);
        }
        (void)munmap(addr, SHM_SIZE);
        (void)shm_unlink(shm_name);
        return EXIT_FAILURE;
    }

    /* d) Cleanup */
    if (munmap(addr, SHM_SIZE) != 0) die_errno("munmap");
    if (shm_unlink(shm_name) != 0) die_errno("shm_unlink");

    return EXIT_SUCCESS;
}