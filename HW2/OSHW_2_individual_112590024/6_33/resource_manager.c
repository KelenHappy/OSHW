#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_RESOURCES 5
//(a) 可能會造成 race condition
int available_resources = MAX_RESOURCES;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

int decrease_count(int count)
{
    pthread_mutex_lock(&mutex);
    //改成 while
    while (available_resources < count) {
        pthread_cond_wait(&cond, &mutex); /* atomically releases lock & sleeps */
    }

    available_resources -= count;
    pthread_mutex_unlock(&mutex);
    return 0;
}
//(c)用mutex lock 把它鎖住
int increase_count(int count)
{
    pthread_mutex_lock(&mutex);

    available_resources += count;
    // 打開全每個thread，每一個會重新確認 while
    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&mutex);
    return 0;
}
