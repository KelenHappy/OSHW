/*
 *   gcc pid_ex.c
 *   ./a
 */

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ── PID manager constants ─────────────────────────────────────────────── */
#define MIN_PID   300
#define MAX_PID  5000

/* ── Test parameters ───────────────────────────────────────────────────── */
#define NUM_THREADS  100   /* threads to spawn                             */
#define MAX_SLEEP      5   /* upper bound on random sleep, seconds         */

/* ── Internal bitmap state ─────────────────────────────────────────────── */
static uint8_t  *g_bitmap      = NULL;
static size_t    g_num_pids    = 0;
static size_t    g_num_bytes   = 0;
static int       g_initialized = 0;

/* ── Mutex protecting all PID-manager operations ───────────────────────── */
static pthread_mutex_t pid_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ── Statistics (also protected by pid_mutex) ──────────────────────────── */
static int g_success_count = 0;   /* threads that obtained a PID          */
static int g_fail_count    = 0;   /* threads that found the pool exhausted*/

/* ═══════════════════════════  PID manager  ═══════════════════════════════ */

static inline void bit_set  (size_t i){ g_bitmap[i>>3] |=  (uint8_t)(1u<<(i&7u)); }
static inline void bit_clear(size_t i){ g_bitmap[i>>3] &= (uint8_t)~(1u<<(i&7u)); }
static inline int  bit_test (size_t i){ return (g_bitmap[i>>3]>>(i&7u))&1u; }

static int find_first_free(size_t *out)
{
    for (size_t b = 0; b < g_num_bytes; ++b) {
        if (g_bitmap[b] == 0xFFu) continue;
        for (unsigned bit = 0; bit < 8; ++bit) {
            size_t idx = (b << 3) + bit;
            if (idx >= g_num_pids) return 0;
            if (!((g_bitmap[b] >> bit) & 1u)) { *out = idx; return 1; }
        }
    }
    return 0;
}

int allocate_map(void)
{
    g_num_pids  = (size_t)(MAX_PID - MIN_PID + 1);
    g_num_bytes = (g_num_pids + 7u) / 8u;
    if (g_bitmap) { memset(g_bitmap, 0, g_num_bytes); g_initialized = 1; return 1; }
    g_bitmap = (uint8_t *)calloc(g_num_bytes, 1);
    if (!g_bitmap) { g_initialized = 0; return -1; }
    g_initialized = 1;
    return 1;
}

/* Thread-safe wrappers */
static int ts_allocate_pid(void)
{
    pthread_mutex_lock(&pid_mutex);
    int pid = -1;
    if (g_initialized) {
        size_t idx;
        if (find_first_free(&idx)) { bit_set(idx); pid = MIN_PID + (int)idx; }
    }
    pthread_mutex_unlock(&pid_mutex);
    return pid;
}

static void ts_release_pid(int pid)
{
    if (pid < MIN_PID || pid > MAX_PID) return;
    pthread_mutex_lock(&pid_mutex);
    bit_clear((size_t)(pid - MIN_PID));
    pthread_mutex_unlock(&pid_mutex);
}

/* ═══════════════════════════  Thread routine  ════════════════════════════ */

static void *thread_routine(void *arg)
{
    long tid = (long)arg;

    /* ── 1. Request a PID ── */
    int pid = ts_allocate_pid();

    if (pid == -1) {
        printf("[thread %3ld] allocate_pid() → FAILED  (pool exhausted)\n", tid);
        pthread_mutex_lock(&pid_mutex);
        ++g_fail_count;
        pthread_mutex_unlock(&pid_mutex);
        return NULL;
    }

    /* Update success counter while mutex is not held (just a counter) */
    pthread_mutex_lock(&pid_mutex);
    ++g_success_count;
    pthread_mutex_unlock(&pid_mutex);

    /* ── 2. Sleep for a random 1–MAX_SLEEP seconds ── */
    int secs = (rand() % MAX_SLEEP) + 1;   /* rand() is seeded per-thread below */
    printf("[thread %3ld] PID %4d acquired → sleeping %d s\n", tid, pid, secs);
    sleep((unsigned)secs);

    /* ── 3. Release the PID ── */
    ts_release_pid(pid);
    printf("[thread %3ld] PID %4d released\n", tid, pid);

    return NULL;
}

/* ═════════════════════════════  main  ════════════════════════════════════ */

int main(void)
{
    /* Seed the global rand() – each thread will use its own seed below. */
    srand((unsigned)time(NULL));

    printf("=== PID Manager – Multithreaded Test ===\n");
    printf("PID range  : [%d, %d]  (%d slots)\n",
           MIN_PID, MAX_PID, MAX_PID - MIN_PID + 1);
    printf("Threads    : %d\n", NUM_THREADS);
    printf("Max sleep  : %d s\n\n", MAX_SLEEP);

    /* Initialise the PID bitmap */
    if (allocate_map() != 1) {
        fprintf(stderr, "allocate_map() failed – aborting.\n");
        return 1;
    }

    /* Create threads */
    pthread_t threads[NUM_THREADS];

    for (long i = 0; i < NUM_THREADS; ++i) {
        int rc = pthread_create(&threads[i], NULL, thread_routine, (void *)i);
        if (rc != 0) {
            fprintf(stderr, "pthread_create failed for thread %ld: %s\n",
                    i, strerror(rc));
            /* Still wait for already-created threads before exiting. */
            for (long j = 0; j < i; ++j) pthread_join(threads[j], NULL);
            return 1;
        }
    }

    /* Join all threads */
    for (int i = 0; i < NUM_THREADS; ++i)
        pthread_join(threads[i], (void **)NULL);

    /* Final report */
    printf("\n=== Results ===\n");
    printf("Successful allocations : %d\n", g_success_count);
    printf("Failed allocations     : %d\n", g_fail_count);
    printf("Total threads          : %d\n", NUM_THREADS);

    /* Sanity check: bitmap should be fully clear (all PIDs released) */
    int leaked = 0;
    for (size_t i = 0; i < g_num_pids; ++i)
        if (bit_test(i)) { ++leaked; }

    if (leaked)
        printf("WARNING: %d PID(s) were NOT released – possible leak!\n", leaked);
    else
        printf("Bitmap clean: all acquired PIDs were properly released.\n");

    free(g_bitmap);
    pthread_mutex_destroy(&pid_mutex);
    return 0;
}
