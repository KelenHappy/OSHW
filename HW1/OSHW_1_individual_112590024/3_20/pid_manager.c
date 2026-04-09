/*
 * Build (library only):
 *   gcc -std=c11 -Wall -Wextra -O2 -c pid_manager.c
 *
 * Build & run tests:
 *   gcc -std=c11 -Wall -Wextra -O2 -DPID_MANAGER_TEST -o pid_manager pid_manager.c
 *   ./pid_manager
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_PID 300
#define MAX_PID 5000

/* ----- internal state ----- */

static uint8_t *g_bitmap = NULL;      /* bits: 1=in use, 0=free */
static size_t g_num_pids = 0;         /* number of pids = MAX_PID - MIN_PID + 1 */
static size_t g_num_bytes = 0;        /* bytes in bitmap */
static int g_initialized = 0;

static inline int pid_in_range(int pid) {
    return pid >= MIN_PID && pid <= MAX_PID;
}

static inline size_t pid_to_index(int pid) {
    /* caller must ensure in range */
    return (size_t)(pid - MIN_PID);
}

static inline void bit_set(size_t idx) {
    g_bitmap[idx >> 3] |= (uint8_t)(1u << (idx & 7u));
}

static inline void bit_clear(size_t idx) {
    g_bitmap[idx >> 3] &= (uint8_t)~(1u << (idx & 7u));
}

static inline int bit_test(size_t idx) {
    return (g_bitmap[idx >> 3] >> (idx & 7u)) & 1u;
}

/* Find first 0-bit; returns 1 if found and sets *out_idx, else 0. */
static int find_first_free(size_t *out_idx) {
    if (!g_initialized || g_bitmap == NULL) return 0;

    for (size_t byte_i = 0; byte_i < g_num_bytes; ++byte_i) {
        uint8_t b = g_bitmap[byte_i];

        /* If all bits are 1, no free PID in that byte. */
        if (b == 0xFFu) continue;

        /* Otherwise find the first 0 bit within this byte. */
        for (unsigned bit = 0; bit < 8; ++bit) {
            size_t idx = (byte_i << 3) + bit;
            if (idx >= g_num_pids) break; /* last byte may be partially used */
            if (((b >> bit) & 1u) == 0u) {
                *out_idx = idx;
                return 1;
            }
        }
    }
    return 0;
}

/* ----- public API ----- */

int allocate_map(void) {
    /* Initialize or reinitialize the bitmap. */
    g_num_pids = (size_t)(MAX_PID - MIN_PID + 1);
    if (g_num_pids == 0) {
        return -1;
    }

    g_num_bytes = (g_num_pids + 7u) / 8u;

    /* If already initialized, reset the bitmap to 0 (all free). */
    if (g_bitmap != NULL) {
        memset(g_bitmap, 0, g_num_bytes);
        g_initialized = 1;
        return 1;
    }

    g_bitmap = (uint8_t *)calloc(g_num_bytes, 1);
    if (!g_bitmap) {
        g_initialized = 0;
        return -1;
    }

    g_initialized = 1;
    return 1;
}

int allocate_pid(void) {
    if (!g_initialized || g_bitmap == NULL) {
        /* Not initialized: treat as failure. */
        return -1;
    }

    size_t idx;
    if (!find_first_free(&idx)) {
        return -1; /* all in use */
    }

    bit_set(idx);
    return (int)(MIN_PID + (int)idx);
}

void release_pid(int pid) {
    if (!g_initialized || g_bitmap == NULL) return;
    if (!pid_in_range(pid)) return;

    size_t idx = pid_to_index(pid);
    bit_clear(idx);
}

/* Optional: free resources (not required by assignment API) */
static void free_map_internal(void) {
    free(g_bitmap);
    g_bitmap = NULL;
    g_num_pids = 0;
    g_num_bytes = 0;
    g_initialized = 0;
}

/* ----- small test driver ----- */
#ifdef PID_MANAGER_TEST

static void die(const char *msg) {
    fprintf(stderr, "TEST ERROR: %s\n", msg);
    exit(1);
}

int main(void) {
    if (allocate_map() != 1) die("allocate_map failed");
    printf("allocate_map(): OK, range [%d, %d]\n", MIN_PID, MAX_PID);

    /* Allocate a few PIDs; should be sequential from MIN_PID upward. */
    int p1 = allocate_pid();
    int p2 = allocate_pid();
    int p3 = allocate_pid();

    if (p1 != MIN_PID) die("first pid not MIN_PID");
    if (p2 != MIN_PID + 1) die("second pid not MIN_PID+1");
    if (p3 != MIN_PID + 2) die("third pid not MIN_PID+2");

    printf("Allocated: %d, %d, %d\n", p1, p2, p3);

    /* Release middle and allocate again; should reuse released pid. */
    release_pid(p2);
    int p4 = allocate_pid();
    if (p4 != p2) die("did not reuse released pid");
    printf("Released %d, reallocated %d: OK\n", p2, p4);

    /* Exhaust the pool quickly by allocating until -1. */
    int count = 0;
    while (allocate_pid() != -1) {
        count++;
        /* avoid infinite loop if bug occurs */
        if (count > (MAX_PID - MIN_PID + 10)) die("allocation did not stop");
    }
    printf("Exhaustion test: OK (allocated remaining %d PIDs until full)\n", count);

    /* After exhaustion, allocate_pid must return -1 */
    if (allocate_pid() != -1) die("allocate_pid should fail when full");
    printf("allocate_pid() when full: OK\n");

    /* Releasing an out-of-range pid should do nothing / not crash. */
    release_pid(MIN_PID - 1);
    release_pid(MAX_PID + 1);
    printf("release_pid() out-of-range: OK\n");

    free_map_internal();
    printf("All tests passed.\n");
    return 0;
}

#endif