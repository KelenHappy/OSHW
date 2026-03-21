#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>

#define PROC_NAME "jiffies"

static int jiffies_show(struct seq_file *m, void *v)
{
    seq_printf(m, "%lu\n", jiffies);
    return 0;
}

static int jiffies_open(struct inode *inode, struct file *file)
{
    return single_open(file, jiffies_show, NULL);
}

static const struct proc_ops jiffies_proc_ops = {
    .proc_open    = jiffies_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static struct proc_dir_entry *proc_entry;

static int __init jiffies_init(void)
{
    proc_entry = proc_create(PROC_NAME, 0444, NULL, &jiffies_proc_ops);
    if (!proc_entry)
        return -ENOMEM;

    pr_info("jiffies_proc: created /proc/%s\n", PROC_NAME);
    return 0;
}

static void __exit jiffies_exit(void)
{
    proc_remove(proc_entry);
    pr_info("jiffies_proc: removed /proc/%s\n", PROC_NAME);
}

module_init(jiffies_init);
module_exit(jiffies_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kelen");
MODULE_DESCRIPTION("Expose jiffies via /proc/jiffies");
