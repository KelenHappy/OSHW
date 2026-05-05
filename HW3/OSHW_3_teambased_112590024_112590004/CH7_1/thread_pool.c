/*
 * Project 1: Thread Pool
 *
 * gcc thread_pool.c -o thread_pool -lpthread
 * ./thread_pool
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_THREADS  4    /* worker thread 數量 */
#define QUEUE_SIZE   10   /* task queue 大小 */

/* ── Task 結構 ── */
typedef struct {
    void (*function)(void *);  /* 要執行的函式 */
    void *arg;                 /* 函式的參數 */
} Task;

/* ── Task Queue ── */
static Task   queue[QUEUE_SIZE];
static int    queue_head = 0;
static int    queue_tail = 0;
static int    queue_count = 0;

/* ── POSIX 同步 ── */
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t           task_sem;    /* 通知 worker 有新任務 */

/* ── Thread pool 狀態 ── */
static pthread_t workers[NUM_THREADS];
static int       pool_running = 1;  /* 1=運行中, 0=關閉 */

/* ═══════════════════════  Queue 操作  ═══════════════════════ */

/* 將任務加入 queue */
static int enqueue(Task task)
{
    if (queue_count >= QUEUE_SIZE) return -1;  /* queue 已滿 */
    queue[queue_tail] = task;
    queue_tail = (queue_tail + 1) % QUEUE_SIZE;
    queue_count++;
    return 0;
}

/* 從 queue 取出任務 */
static int dequeue(Task *task)
{
    if (queue_count == 0) return -1;  /* queue 為空 */
    *task = queue[queue_head];
    queue_head = (queue_head + 1) % QUEUE_SIZE;
    queue_count--;
    return 0;
}

/* ═══════════════════════  Worker Thread  ═══════════════════════ */

static void *worker(void *arg)
{
    long id = (long)arg;

    while (1) {
        /* 等待任務通知 */
        sem_wait(&task_sem);

        /* 如果 pool 關閉且 queue 為空則結束 */
        if (!pool_running) {
            pthread_mutex_lock(&queue_mutex);
            int empty = (queue_count == 0);
            pthread_mutex_unlock(&queue_mutex);
            if (empty) break;
        }

        /* 從 queue 取出任務 */
        pthread_mutex_lock(&queue_mutex);
        Task task;
        int got = dequeue(&task);
        pthread_mutex_unlock(&queue_mutex);

        /* 執行任務 */
        if (got == 0) {
            printf("[Worker %ld] executing task...\n", id);
            task.function(task.arg);
        }
    }

    printf("[Worker %ld] shutting down.\n", id);
    return NULL;
}

/* ═══════════════════════  Public API  ═══════════════════════ */

/* 初始化 thread pool */
void pool_init(void)
{
    sem_init(&task_sem, 0, 0);

    for (long i = 0; i < NUM_THREADS; i++)
        pthread_create(&workers[i], NULL, worker, (void *)i);

    printf("Thread pool initialized with %d workers.\n\n", NUM_THREADS);
}

/* 提交任務到 thread pool */
int pool_submit(void (*function)(void *), void *arg)
{
    Task task = { function, arg };

    pthread_mutex_lock(&queue_mutex);
    int ret = enqueue(task);
    pthread_mutex_unlock(&queue_mutex);

    if (ret == 0)
        sem_post(&task_sem);  /* 通知 worker 有新任務 */
    else
        printf("[pool_submit] queue is full, task rejected.\n");

    return ret;
}

/* 關閉 thread pool */
void pool_shutdown(void)
{
    pool_running = 0;

    /* 喚醒所有 worker 讓它們結束 */
    for (int i = 0; i < NUM_THREADS; i++)
        sem_post(&task_sem);

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(workers[i], NULL);

    sem_destroy(&task_sem);
    pthread_mutex_destroy(&queue_mutex);

    printf("\nThread pool shutdown complete.\n");
}

/* ═══════════════════════  測試任務  ═══════════════════════ */

void sample_task(void *arg)
{
    int id = *(int *)arg;
    printf("[Task %d] running, sleeping 1s...\n", id);
    sleep(1);
    printf("[Task %d] done.\n", id);
    free(arg);
}

/* ═══════════════════════  main  ═══════════════════════ */

int main(void)
{
    pool_init();

    /* 提交 8 個任務 */
    for (int i = 0; i < 8; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pool_submit(sample_task, id);
        printf("[Main] submitted task %d\n", i);
    }

    /* 等待所有任務完成後關閉 */
    sleep(5);
    pool_shutdown();

    return 0;
}