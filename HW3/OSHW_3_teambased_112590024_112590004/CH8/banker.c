/*
 * gcc banker.c -o banker -lpthread
 * ./banker 10 5 7
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_CUSTOMERS  5   /* customer 數量 */
#define MAX_LOOP       3   /* 每個 customer 請求次數 */

/* ── 資源種類數量（從 command line 決定）── */
static int num_resources;

/* ── Banker's Algorithm 資料結構 ── */
static int *available;              /* 可用資源 */
static int  maximum[NUM_CUSTOMERS][10];    /* 最大需求 */
static int  allocation[NUM_CUSTOMERS][10]; /* 已分配 */
static int  need[NUM_CUSTOMERS][10];       /* 還需要 */

/* ── mutex 保護共享資料 ── */
static pthread_mutex_t bank_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ═══════════════════  Safety Algorithm  ═══════════════════ */

static int is_safe(void)
{
    int work[10], finish[NUM_CUSTOMERS];

    /* 初始化 work = available, finish = false */
    for (int i = 0; i < num_resources; i++)
        work[i] = available[i];
    for (int i = 0; i < NUM_CUSTOMERS; i++)
        finish[i] = 0;

    /* 找出可以完成的 customer */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < NUM_CUSTOMERS; i++) {
            if (finish[i]) continue;

            /* 檢查 need[i] <= work */
            int ok = 1;
            for (int j = 0; j < num_resources; j++) {
                if (need[i][j] > work[j]) { ok = 0; break; }
            }

            if (ok) {
                /* 模擬釋放資源 */
                for (int j = 0; j < num_resources; j++)
                    work[j] += allocation[i][j];
                finish[i] = 1;
                changed = 1;
            }
        }
    }

    /* 全部 finish 才是 safe */
    for (int i = 0; i < NUM_CUSTOMERS; i++)
        if (!finish[i]) return 0;
    return 1;
}

/* ═══════════════════  Request Resources  ═══════════════════ */

static int request_resources(int customer_id, int *request)
{
    pthread_mutex_lock(&bank_mutex);

    /* Step 1: request <= need? */
    for (int i = 0; i < num_resources; i++) {
        if (request[i] > need[customer_id][i]) {
            printf("[Customer %d] ERROR: request exceeds need\n", customer_id);
            pthread_mutex_unlock(&bank_mutex);
            return -1;
        }
    }

    /* Step 2: request <= available? */
    for (int i = 0; i < num_resources; i++) {
        if (request[i] > available[i]) {
            printf("[Customer %d] request denied: not enough resources\n", customer_id);
            pthread_mutex_unlock(&bank_mutex);
            return -1;
        }
    }

    /* Step 3: 假設分配，檢查 safety */
    for (int i = 0; i < num_resources; i++) {
        available[i]               -= request[i];
        allocation[customer_id][i] += request[i];
        need[customer_id][i]       -= request[i];
    }

    if (!is_safe()) {
        /* 不安全，回滾 */
        for (int i = 0; i < num_resources; i++) {
            available[i]               += request[i];
            allocation[customer_id][i] -= request[i];
            need[customer_id][i]       += request[i];
        }
        printf("[Customer %d] request denied: unsafe state\n", customer_id);
        pthread_mutex_unlock(&bank_mutex);
        return -1;
    }

    printf("[Customer %d] request granted\n", customer_id);
    pthread_mutex_unlock(&bank_mutex);
    return 0;
}

/* ═══════════════════  Release Resources  ═══════════════════ */

static void release_resources(int customer_id, int *release)
{
    pthread_mutex_lock(&bank_mutex);

    for (int i = 0; i < num_resources; i++) {
        /* 不能釋放超過已分配的量 */
        if (release[i] > allocation[customer_id][i])
            release[i] = allocation[customer_id][i];

        available[i]               += release[i];
        allocation[customer_id][i] -= release[i];
        need[customer_id][i]       += release[i];
    }

    printf("[Customer %d] released resources\n", customer_id);
    pthread_mutex_unlock(&bank_mutex);
}

/* ═══════════════════  Customer Thread  ═══════════════════ */

static void *customer(void *arg)
{
    int id = *(int *)arg;
    int request[10], release[10];

    for (int loop = 0; loop < MAX_LOOP; loop++) {
        /* 產生隨機請求（不超過 need）*/
        pthread_mutex_lock(&bank_mutex);
        for (int i = 0; i < num_resources; i++)
            request[i] = (need[id][i] > 0) ? (rand() % (need[id][i] + 1)) : 0;
        pthread_mutex_unlock(&bank_mutex);

        request_resources(id, request);
        sleep(1);

        /* 產生隨機釋放（不超過 allocation）*/
        pthread_mutex_lock(&bank_mutex);
        for (int i = 0; i < num_resources; i++)
            release[i] = (allocation[id][i] > 0) ? (rand() % (allocation[id][i] + 1)) : 0;
        pthread_mutex_unlock(&bank_mutex);

        release_resources(id, release);
        sleep(1);
    }

    printf("[Customer %d] done.\n", id);
    return NULL;
}

/* ═══════════════════  main  ═══════════════════ */

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <resource1> <resource2> ...\n", argv[0]);
        return 1;
    }

    srand((unsigned)time(NULL));

    /* 從 command line 讀取資源數量 */
    num_resources = argc - 1;
    available = malloc(sizeof(int) * num_resources);
    for (int i = 0; i < num_resources; i++)
        available[i] = atoi(argv[i + 1]);

    printf("=== Banker's Algorithm ===\n");
    printf("Customers : %d\n", NUM_CUSTOMERS);
    printf("Resources : %d types -> ", num_resources);
    for (int i = 0; i < num_resources; i++)
        printf("%d ", available[i]);
    printf("\n\n");

    /* 隨機初始化 maximum 和 need */
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        for (int j = 0; j < num_resources; j++) {
            maximum[i][j]    = (available[j] > 0) ? (rand() % available[j] + 1) : 0;
            allocation[i][j] = 0;
            need[i][j]       = maximum[i][j];
        }
    }

    /* 建立 customer threads */
    pthread_t threads[NUM_CUSTOMERS];
    int ids[NUM_CUSTOMERS];
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, customer, &ids[i]);
    }

    /* 等待所有 customer 完成 */
    for (int i = 0; i < NUM_CUSTOMERS; i++)
        pthread_join(threads[i], NULL);

    printf("\n=== All customers done ===\n");

    free(available);
    pthread_mutex_destroy(&bank_mutex);
    return 0;
}