#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/uaccess.h>
#include <linux/pid.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("112590024");
MODULE_DESCRIPTION("Kernel module to display task information via /proc/task_info");

#define PROC_NAME "task_info"
#define MAX_PID_SIZE 16

static struct proc_dir_entry *proc_entry;
static pid_t target_pid = 0;

/* Part II: Reading - Display task information */
static int task_info_show(struct seq_file *m, void *v)
{
	struct task_struct *task;
	struct task_struct *parent;
	char comm[TASK_COMM_LEN];
	pid_t ppid = 0;

	if (target_pid == 0) {
		/* Display current process */
		task = current;
	} else {
		/* Find task by PID */
		task = pid_task(find_vpid(target_pid), PIDTYPE_PID);
		if (task == NULL) {
			seq_printf(m, "Error: PID %d not found\n", target_pid);
			return 0;
		}
	}

	/* Get task information */
	get_task_comm(comm, task);
	parent = rcu_dereference(task->real_parent);
	if (parent != NULL)
		ppid = parent->tgid;

	/* Display task info */
	seq_printf(m, "=== Task Information ===\n");
	seq_printf(m, "PID:      %d\n", task->tgid);
	seq_printf(m, "PPID:     %d\n", ppid);
	seq_printf(m, "Command:  %s\n", comm);
	seq_printf(m, "Priority: %d\n", task->prio);
	seq_printf(m, "State:    ");

	switch (task->__state) {
	case TASK_RUNNING:
		seq_puts(m, "Running\n");
		break;
	case TASK_INTERRUPTIBLE:
		seq_puts(m, "Interruptible\n");
		break;
	case TASK_UNINTERRUPTIBLE:
		seq_puts(m, "Uninterruptible\n");
		break;
	case __TASK_STOPPED:
		seq_puts(m, "Stopped\n");
		break;
	case __TASK_TRACED:
		seq_puts(m, "Traced\n");
		break;
	case TASK_DEAD:
		seq_puts(m, "Dead\n");
		break;
	default:
		seq_printf(m, "Unknown (0x%x)\n", task->__state);
	}

	seq_puts(m, "======================\n");

	return 0;
}

/* Part I: Writing - Accept PID input */
static ssize_t task_info_write(struct file *file, const char __user *buf,
				size_t count, loff_t *pos)
{
	char input_buffer[MAX_PID_SIZE];
	long pid_val;
	int ret;

	if (count >= MAX_PID_SIZE)
		return -EINVAL;

	/* Copy user input */
	if (copy_from_user(input_buffer, buf, count))
		return -EFAULT;

	input_buffer[count] = '\0';

	/* Convert string to PID */
	ret = kstrtol(input_buffer, 10, &pid_val);
	if (ret != 0)
		return -EINVAL;

	/* Store PID (0 means current process) */
	if (pid_val < 0)
		return -EINVAL;

	target_pid = (pid_t)pid_val;

	return count;
}

/* Open handler for seq_file */
static int task_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, task_info_show, NULL);
}

/* File operations structure */
static const struct proc_ops task_info_ops = {
	.proc_open = task_info_open,
	.proc_read = seq_read,
	.proc_write = task_info_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

/* Module initialization */
static int __init task_info_init(void)
{
	proc_entry = proc_create(PROC_NAME, 0666, NULL, &task_info_ops);
	if (proc_entry == NULL) {
		pr_err("Failed to create /proc/%s\n", PROC_NAME);
		return -ENOMEM;
	}

	pr_info("task_info module loaded. Use /proc/%s\n", PROC_NAME);
	pr_info("Write PID to query: echo 1234 > /proc/%s\n", PROC_NAME);
	pr_info("Write 0 to query current process: echo 0 > /proc/%s\n",
		PROC_NAME);

	return 0;
}

/* Module cleanup */
static void __exit task_info_exit(void)
{
	if (proc_entry != NULL)
		proc_remove(proc_entry);

	pr_info("task_info module unloaded\n");
}

module_init(task_info_init);
module_exit(task_info_exit);