//  gcc multi_sort.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>   

typedef struct {
    int *array;    
    int  left;    
    int  right;     
} ThreadArgs;

void  merge_sort(int *arr, int left, int right);
void  merge(int *arr, int left, int mid, int right);
void *thread_sort(void *args);
void  print_array(const char *label, int *arr, int n);

void merge_sort(int *arr, int left, int right)
{
    if (left >= right)
        return;                         /* 只剩一個元素，不需排序 */

    int mid = left + (right - left) / 2;  /* 避免 (left+right) 溢位 */

    merge_sort(arr, left,    mid);      /* 遞迴排序左半部 */
    merge_sort(arr, mid + 1, right);    /* 遞迴排序右半部 */
    merge(arr, left, mid, right);       /* 合併兩個已排序的子陣列 */
}

void merge(int *arr, int left, int mid, int right)
{
    int n = right - left + 1;               /* 這段範圍的元素總數 */

    int *temp = (int *)malloc(n * sizeof(int));
    if (!temp) {
        fprintf(stderr, "記憶體配置失敗\n");
        exit(EXIT_FAILURE);
    }

    int i = left;       
    int j = mid + 1;    
    int k = 0;          

    while (i <= mid && j <= right) {
        if (arr[i] <= arr[j])
            temp[k++] = arr[i++];
        else
            temp[k++] = arr[j++];
    }

    while (i <= mid)
        temp[k++] = arr[i++];

    while (j <= right)
        temp[k++] = arr[j++];

    for (int m = 0; m < n; m++)
        arr[left + m] = temp[m];

    free(temp);
}

void *thread_sort(void *args)
{
    ThreadArgs *t = (ThreadArgs *)args;     

    printf("  [執行緒] 開始排序 index %d ~ %d\n", t->left, t->right);

    merge_sort(t->array, t->left, t->right);

    printf("  [執行緒] 完成排序 index %d ~ %d\n", t->left, t->right);

    return NULL;
}

void print_array(const char *label, int *arr, int n)
{
    printf("%s [ ", label);
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("]\n");
}

int main(void)
{
    int  n;
    int *array;

    printf("請輸入整數個數：");
    scanf("%d", &n);

    if (n < 2) {
        printf("至少需要 2 個整數才能分成兩半。\n");
        return EXIT_FAILURE;
    }

    /* 動態配置陣列 */
    array = (int *)malloc(n * sizeof(int));
    if (!array) {
        fprintf(stderr, "記憶體配置失敗\n");
        return EXIT_FAILURE;
    }

    printf("請輸入 %d 個整數（以空格分隔）：", n);
    for (int i = 0; i < n; i++)
        scanf("%d", &array[i]);

    print_array("\n排序前：", array, n);

    int mid = (n - 1) / 2;

    printf("\n切割方式：\n");
    printf("  左半部 index  0 ~ %d（%d 個元素）\n", mid, mid + 1);
    printf("  右半部 index %d ~ %d（%d 個元素）\n\n", mid + 1, n - 1, n - 1 - mid);

    ThreadArgs args1 = { array, 0,       mid   };   /* Thread 1：左半部 */
    ThreadArgs args2 = { array, mid + 1, n - 1 };   /* Thread 2：右半部 */

    pthread_t thread1, thread2;

    printf("啟動執行緒...\n");

    pthread_create(&thread1, NULL, thread_sort, &args1);
    pthread_create(&thread2, NULL, thread_sort, &args2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("\n兩個執行緒皆已完成。\n");

    /* 印出各半部排序後的狀態（方便觀察） */
    print_array("左半部排序後：", array,        mid + 1);
    print_array("右半部排序後：", array + mid + 1, n - 1 - mid);

    printf("\n主執行緒執行最終合併...\n");
    merge(array, 0, mid, n - 1);

    print_array("排序後：", array, n);

    free(array);

    printf("\n程式結束。\n");
    return EXIT_SUCCESS;
}