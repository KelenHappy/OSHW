#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("112590024");
MODULE_DESCRIPTION("Linear iteration of all tasks using for_each_process()");
MODULE_VERSION("1.0");

static int __init task_init(void) {
    struct task_struct *task;
    int task_count = 0;
    
    printk(KERN_INFO "==========================================\n");
    printk(KERN_INFO "Listing All Tasks - Linear Iteration\n");
    printk(KERN_INFO "==========================================\n");
    printk(KERN_INFO "%-12s %-25s %-20s\n", "PID", "Name", "State");
    printk(KERN_INFO "------------------------------------------\n");
    
    for_each_process(task) {
        const char *state_str = "UNKNOWN";
        
        switch (task->__state) {
            case TASK_RUNNING:
                state_str = "RUNNING";
                break;
            case TASK_INTERRUPTIBLE:
                state_str = "INTERRUPTIBLE";
                break;
            case TASK_UNINTERRUPTIBLE:
                state_str = "UNINTERRUPTIBLE";
                break;
            case __TASK_STOPPED:
                state_str = "STOPPED";
                break;
            case __TASK_TRACED:
                state_str = "TRACED";
                break;
            case TASK_DEAD:
                state_str = "DEAD";
                break;
            case TASK_WAKEKILL:
                state_str = "WAKEKILL";
                break;
            case TASK_WAKING:
                state_str = "WAKING";
                break;
            default:
                state_str = "UNKNOWN";
        }
        
        printk(KERN_INFO "%-12d %-25s %-20s\n", 
               task->pid, 
               task->comm, 
               state_str);
        task_count++;
    }
    
    printk(KERN_INFO "------------------------------------------\n");
    printk(KERN_INFO "Total tasks found: %d\n", task_count);
    printk(KERN_INFO "==========================================\n");
    
    return 0;
}

static void __exit task_exit(void) {
    printk(KERN_INFO "Task listing module (linear) unloaded\n");
}

module_init(task_init);
module_exit(task_exit);