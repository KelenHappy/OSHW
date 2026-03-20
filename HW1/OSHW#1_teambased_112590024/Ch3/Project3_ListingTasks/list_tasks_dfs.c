/*
 * list_tasks_dfs.c
 * 
 * Project 3 - Part II: Listing All Tasks (Depth-First Search Traversal)
 * 
 * This kernel module traverses the process tree in a depth-first manner
 * starting from init_task and outputs the tree structure to the kernel log.
 * The tree structure shows parent-child relationships through indentation.
 * 
 * Usage:
 *   make
 *   insmod list_tasks_dfs.ko
 *   dmesg | tail -n 200
 *   rmmod list_tasks_dfs
 * 
 * Tested on: Linux 6.19+
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("112590024");
MODULE_DESCRIPTION("Kernel module to list all tasks using DFS tree traversal");

/* Helper function to get process state as string */
static const char *get_state_string(long state)
{
    switch (state) {
        case TASK_RUNNING:
            return "RUNNING";
        case TASK_INTERRUPTIBLE:
            return "INTERRUPTIBLE";
        case TASK_UNINTERRUPTIBLE:
            return "UNINTERRUPTIBLE";
        case TASK_STOPPED:
            return "STOPPED";
        case TASK_TRACED:
            return "TRACED";
        case EXIT_DEAD:
            return "EXIT_DEAD";
        case EXIT_ZOMBIE:
            return "EXIT_ZOMBIE";
        default:
            return "UNKNOWN";
    }
}

/* DFS traversal function - recursively traverse process tree */
static void dfs_traverse(struct task_struct *task, int depth, int *count)
{
    struct task_struct *child;
    struct list_head *list;
    char indent[128];
    int i;

    if (depth > 100) {
        printk(KERN_INFO "Max depth reached, stopping traversal\n");
        return;  /* Prevent excessive recursion */
    }

    (*count)++;

    /* Build indentation string for tree visualization */
    indent[0] = '\0';
    for (i = 0; i < depth * 2; i++) {
        strncat(indent, " ", sizeof(indent) - strlen(indent) - 1);
    }

    /* Print current task with tree structure */
    if (depth == 0) {
        /* Root task (init) */
        printk(KERN_INFO "%s[%5d] %-20s %-20s (root)\n",
               indent,
               task->pid,
               task->comm,
               get_state_string(task->__state));
    } else {
        /* Child tasks with tree branch character */
        printk(KERN_INFO "%s├─ [%5d] %-20s %-20s\n",
               indent,
               task->pid,
               task->comm,
               get_state_string(task->__state));
    }

    /* Recursively traverse all children of current task */
    list_for_each(list, &task->children) {
        child = list_entry(list, struct task_struct, sibling);
        dfs_traverse(child, depth + 1, count);
    }
}

/* Module initialization function */
static int __init list_tasks_dfs_init(void)
{
    int count = 0;

    printk(KERN_INFO "==========================================\n");
    printk(KERN_INFO "Task Lister - DFS Tree Traversal (Part II)\n");
    printk(KERN_INFO "==========================================\n");
    printk(KERN_INFO "Process Tree Structure:\n");
    printk(KERN_INFO "[  PID] Command              State               \n");
    printk(KERN_INFO "==========================================\n");

    /* Start DFS traversal from init_task (PID 1) */
    dfs_traverse(&init_task, 0, &count);

    /* Summary statistics */
    printk(KERN_INFO "==========================================\n");
    printk(KERN_INFO "Total tasks found: %d\n", count);
    printk(KERN_INFO "==========================================\n");
    printk(KERN_INFO "DFS tree traversal completed successfully!\n");

    return 0;
}

/* Module cleanup function */
static void __exit list_tasks_dfs_exit(void)
{
    printk(KERN_INFO "Task Lister DFS module unloaded.\n");
}

module_init(list_tasks_dfs_init);
module_exit(list_tasks_dfs_exit);