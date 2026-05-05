/*
 * 8.32: Single-Lane Bridge with POSIX Synchronization
 *
 * gcc posix.c -o posix -lpthread
 * ./posix
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* ── mutex 保護橋，確保同一時間只有一個方向能過橋 ── */
static pthread_mutex_t bridge_lock = PTHREAD_MUTEX_INITIALIZER;

/* ── 北向農夫執行緒 ── */
static void *northbound_farmer(void *arg)
{
    long id = (long)arg;

    printf("[Northbound %ld] waiting to cross...\n", id);

    /* 取得橋的控制權 */
    pthread_mutex_lock(&bridge_lock);

    printf("[Northbound %ld] on the bridge, crossing...\n", id);

    /* 隨機睡眠 1~3 秒，模擬過橋時間 */
    sleep((rand() % 3) + 1);

    printf("[Northbound %ld] crossed the bridge.\n", id);

    /* 釋放橋的控制權 */
    pthread_mutex_unlock(&bridge_lock);

    return NULL;
}

/* ── 南向農夫執行緒 ── */
static void *southbound_farmer(void *arg)
{
    long id = (long)arg;

    printf("[Southbound %ld] waiting to cross...\n", id);

    /* 取得橋的控制權 */
    pthread_mutex_lock(&bridge_lock);

    printf("[Southbound %ld] on the bridge, crossing...\n", id);

    /* 隨機睡眠 1~3 秒，模擬過橋時間 */
    sleep((rand() % 3) + 1);

    printf("[Southbound %ld] crossed the bridge.\n", id);

    /* 釋放橋的控制權 */
    pthread_mutex_unlock(&bridge_lock);

    return NULL;
}

int main(void)
{
    srand((unsigned)time(NULL));

    int num_north, num_south;

    /* 讓用戶輸入農夫數量 */
    printf("Enter number of northbound farmers: ");
    scanf("%d", &num_north);

    printf("Enter number of southbound farmers: ");
    scanf("%d", &num_south);

    /* 建立執行緒陣列 */
    pthread_t *north_threads = malloc(sizeof(pthread_t) * num_north);
    pthread_t *south_threads = malloc(sizeof(pthread_t) * num_south);

    printf("\n=== Bridge Simulation Start ===\n\n");

    /* 建立北向農夫執行緒 */
    for (long i = 0; i < num_north; i++)
        pthread_create(&north_threads[i], NULL, northbound_farmer, (void *)i);

    /* 建立南向農夫執行緒 */
    for (long i = 0; i < num_south; i++)
        pthread_create(&south_threads[i], NULL, southbound_farmer, (void *)i);

    /* 等待所有執行緒完成 */
    for (int i = 0; i < num_north; i++)
        pthread_join(north_threads[i], NULL);

    for (int i = 0; i < num_south; i++)
        pthread_join(south_threads[i], NULL);

    printf("\n=== All farmers have crossed the bridge ===\n");

    free(north_threads);
    free(south_threads);
    pthread_mutex_destroy(&bridge_lock);

    return 0;
}