#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 100
#define QUANTUM 10 // Round-Robin 的時間片段

typedef struct task {
    char name[10];
    int priority;
    int burst;
    int remaining_burst; // 用於 RR 和 Priority-RR
} Task;

Task *taskList[100];
int taskCount = 0;

// 模擬 CPU 執行函數
void run(Task *t, int slice) {
    printf("Running task = [%s] [Priority: %d] for %d units.\n", t->name, t->priority, slice);
}

// 讀取檔案並解析任務
void loadTasks(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("無法開啟輸入檔案");
        exit(1);
    }

    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        Task *t = malloc(sizeof(Task));
        char *temp = strdup(line);
        char *to_free = temp; 
        
        char *n = strsep(&temp, ",");
        char *p = strsep(&temp, ",");
        char *b = strsep(&temp, ",");

        if (n && p && b) {
            strcpy(t->name, n);
            t->priority = atoi(p);
            t->burst = atoi(b);
            t->remaining_burst = t->burst;
            taskList[taskCount++] = t;
        } else {
            free(t);
        }
        free(to_free); 
    }
    fclose(fp);
}

// 1. FCFS (先來先服務)
void fcfs() {
    printf("\n--- FCFS Scheduling ---\n");
    for (int i = 0; i < taskCount; i++) {
        run(taskList[i], taskList[i]->burst);
    }
}

// 2. SJF (最短工作優先)
void sjf() {
    printf("\n--- SJF Scheduling ---\n");
    // 根據 Burst Time 由小到大排序
    for (int i = 0; i < taskCount - 1; i++) {
        for (int j = i + 1; j < taskCount; j++) {
            if (taskList[i]->burst > taskList[j]->burst) {
                Task *temp = taskList[i];
                taskList[i] = taskList[j];
                taskList[j] = temp;
            }
        }
    }
    for (int i = 0; i < taskCount; i++) {
        run(taskList[i], taskList[i]->burst);
    }
}

// 3. Priority (優先權排程)
void priority_sched() {
    printf("\n--- Priority Scheduling ---\n");
    for (int i = 0; i < taskCount - 1; i++) {
        for (int j = i + 1; j < taskCount; j++) {
            if (taskList[i]->priority < taskList[j]->priority) {
                Task *temp = taskList[i];
                taskList[i] = taskList[j];
                taskList[j] = temp;
            }
        }
    }
    for (int i = 0; i < taskCount; i++) {
        run(taskList[i], taskList[i]->burst);
    }
}

// 4. Round-Robin (RR)
void rr() {
    printf("\n--- Round-Robin Scheduling (Quantum = %d) ---\n", QUANTUM);
    int finished = 0;
    while (finished < taskCount) {
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i]->remaining_burst > 0) {
                int slice = (taskList[i]->remaining_burst > QUANTUM) ? QUANTUM : taskList[i]->remaining_burst;
                run(taskList[i], slice);
                taskList[i]->remaining_burst -= slice;
                if (taskList[i]->remaining_burst == 0) finished++;
            }
        }
    }
}

// 5. Priority with Round-Robin (優先權相同時使用 RR)
void priority_rr() {
    printf("\n--- Priority with RR Scheduling (Quantum = %d) ---\n", QUANTUM);
    // 先按優先權排序
    for (int i = 0; i < taskCount - 1; i++) {
        for (int j = i + 1; j < taskCount; j++) {
            if (taskList[i]->priority < taskList[j]->priority) {
                Task *temp = taskList[i];
                taskList[i] = taskList[j];
                taskList[j] = temp;
            }
        }
    }

    int i = 0;
    while (i < taskCount) {
        int start = i, end = i;
        // 找出同一優先權的區間 [start, end)
        while (end < taskCount && taskList[end]->priority == taskList[start]->priority) {
            end++;
        }
        
        int group_size = end - start;
        int group_finished = 0;
        // 對該優先權組進行 RR
        while (group_finished < group_size) {
            for (int j = start; j < end; j++) {
                if (taskList[j]->remaining_burst > 0) {
                    int slice = (taskList[j]->remaining_burst > QUANTUM) ? QUANTUM : taskList[j]->remaining_burst;
                    run(taskList[j], slice);
                    taskList[j]->remaining_burst -= slice;
                    if (taskList[j]->remaining_burst == 0) group_finished++;
                }
            }
        }
        i = end;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("用法: %s [schedule.txt] [演算法編號]\n", argv[0]);
        printf("1: FCFS, 2: SJF, 3: Priority, 4: RR, 5: Priority-RR\n");
        return 1;
    }

    loadTasks(argv[1]);
    int choice = atoi(argv[2]);

    switch (choice) {
        case 1: fcfs(); break;
        case 2: sjf(); break;
        case 3: priority_sched(); break;
        case 4: rr(); break;
        case 5: priority_rr(); break;
        default: printf("請輸入 1 到 5 之間的編號\n");
    }

    // 清理記憶體
    for (int i = 0; i < taskCount; i++) free(taskList[i]);

    return 0;
}