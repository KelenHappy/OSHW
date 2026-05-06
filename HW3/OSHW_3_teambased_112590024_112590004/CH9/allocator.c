/*
 * gcc allocator.c -o allocator
 * ./allocator 1048576
 *
 * Commands:
 *   RQ <process> <size> <F|B|W>  - Request memory
 *   RL <process>                  - Release memory
 *   C                             - Compact memory
 *   STAT                          - Report memory status
 *   X                             - Exit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HOLES 100  /* 最大記憶體區塊數量 */

/* ── 記憶體區塊結構 ── */
typedef struct {
    int  start;           /* 起始位址 */
    int  size;            /* 大小 */
    int  is_free;         /* 1=空閒, 0=已分配 */
    char process[16];     /* 行程名稱 */
} Hole;

static Hole  holes[MAX_HOLES];  /* 記憶體區塊陣列 */
static int   num_holes = 0;     /* 區塊數量 */
static int   max_memory = 0;    /* 總記憶體大小 */

/* ═══════════════════  初始化  ═══════════════════ */

void init_memory(int size)
{
    max_memory = size;
    num_holes  = 1;
    holes[0].start  = 0;
    holes[0].size   = size;
    holes[0].is_free = 1;
    strcpy(holes[0].process, "");
}

/* ═══════════════════  STAT  ═══════════════════ */

void stat_memory(void)
{
    printf("\n--- Memory Status ---\n");
    for (int i = 0; i < num_holes; i++) {
        if (holes[i].is_free)
            printf("Addresses [%d:%d] Unused\n",
                   holes[i].start, holes[i].start + holes[i].size - 1);
        else
            printf("Addresses [%d:%d] Process %s\n",
                   holes[i].start, holes[i].start + holes[i].size - 1,
                   holes[i].process);
    }
    printf("---------------------\n\n");
}

/* ═══════════════════  Request Memory  ═══════════════════ */

void request_memory(char *process, int size, char algo)
{
    int target = -1;

    if (algo == 'F') {
        /* First-fit: 找第一個夠大的空洞 */
        for (int i = 0; i < num_holes; i++) {
            if (holes[i].is_free && holes[i].size >= size) {
                target = i;
                break;
            }
        }
    } else if (algo == 'B') {
        /* Best-fit: 找最小但夠大的空洞 */
        int best_size = max_memory + 1;
        for (int i = 0; i < num_holes; i++) {
            if (holes[i].is_free && holes[i].size >= size &&
                holes[i].size < best_size) {
                best_size = holes[i].size;
                target = i;
            }
        }
    } else if (algo == 'W') {
        /* Worst-fit: 找最大的空洞 */
        int worst_size = -1;
        for (int i = 0; i < num_holes; i++) {
            if (holes[i].is_free && holes[i].size >= size &&
                holes[i].size > worst_size) {
                worst_size = holes[i].size;
                target = i;
            }
        }
    }

    if (target == -1) {
        printf("Error: No sufficient memory for %s\n", process);
        return;
    }

    /* 分割區塊 */
    int remaining = holes[target].size - size;

    holes[target].is_free = 0;
    holes[target].size    = size;
    strcpy(holes[target].process, process);

    /* 若有剩餘空間，插入新的空閒區塊 */
    if (remaining > 0) {
        /* 後面的區塊往後移 */
        for (int i = num_holes; i > target + 1; i--)
            holes[i] = holes[i - 1];
        num_holes++;

        holes[target + 1].start   = holes[target].start + size;
        holes[target + 1].size    = remaining;
        holes[target + 1].is_free = 1;
        strcpy(holes[target + 1].process, "");
    }

    printf("Allocated %d bytes for %s at address %d\n",
           size, process, holes[target].start);
}

/* ═══════════════════  Release Memory  ═══════════════════ */

void release_memory(char *process)
{
    int found = 0;

    for (int i = 0; i < num_holes; i++) {
        if (!holes[i].is_free && strcmp(holes[i].process, process) == 0) {
            holes[i].is_free = 1;
            strcpy(holes[i].process, "");
            found = 1;
            printf("Released memory for %s\n", process);

            /* 合併相鄰空閒區塊 */
            /* 合併右邊 */
            if (i + 1 < num_holes && holes[i + 1].is_free) {
                holes[i].size += holes[i + 1].size;
                for (int j = i + 1; j < num_holes - 1; j++)
                    holes[j] = holes[j + 1];
                num_holes--;
            }
            /* 合併左邊 */
            if (i > 0 && holes[i - 1].is_free) {
                holes[i - 1].size += holes[i].size;
                for (int j = i; j < num_holes - 1; j++)
                    holes[j] = holes[j + 1];
                num_holes--;
            }
            break;
        }
    }

    if (!found)
        printf("Error: Process %s not found\n", process);
}

/* ═══════════════════  Compact Memory  ═══════════════════ */

void compact_memory(void)
{
    /* 把所有已分配的區塊往前移，空閒空間合併到最後 */
    int current = 0;
    int free_size = 0;

    for (int i = 0; i < num_holes; i++) {
        if (holes[i].is_free) {
            free_size += holes[i].size;
        } else {
            holes[current].start   = (current == 0) ? 0 : holes[current - 1].start + holes[current - 1].size;
            holes[current].size    = holes[i].size;
            holes[current].is_free = 0;
            strcpy(holes[current].process, holes[i].process);
            current++;
        }
    }

    /* 加入合併後的空閒區塊 */
    if (free_size > 0) {
        holes[current].start   = (current == 0) ? 0 : holes[current - 1].start + holes[current - 1].size;
        holes[current].size    = free_size;
        holes[current].is_free = 1;
        strcpy(holes[current].process, "");
        num_holes = current + 1;
    } else {
        num_holes = current;
    }

    printf("Memory compacted. Free space consolidated.\n");
}

/* ═══════════════════  main  ═══════════════════ */

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <memory_size>\n", argv[0]);
        return 1;
    }

    int mem_size = atoi(argv[1]);
    init_memory(mem_size);

    printf("=== Contiguous Memory Allocator ===\n");
    printf("Total Memory: %d bytes\n\n", mem_size);

    char line[256];
    while (1) {
        printf("allocator> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        /* 去掉換行 */
        line[strcspn(line, "\n")] = 0;

        char cmd[16], process[16], algo;
        int  size;

        if (strcmp(line, "X") == 0) {
            printf("Exiting.\n");
            break;
        } else if (strcmp(line, "STAT") == 0) {
            stat_memory();
        } else if (strcmp(line, "C") == 0) {
            compact_memory();
        } else if (sscanf(line, "RQ %s %d %c", process, &size, &algo) == 3) {
            request_memory(process, size, algo);
        } else if (sscanf(line, "RL %s", process) == 1) {
            release_memory(process);
        } else {
            printf("Unknown command. Use: RQ, RL, C, STAT, X\n");
        }
    }

    return 0;
}