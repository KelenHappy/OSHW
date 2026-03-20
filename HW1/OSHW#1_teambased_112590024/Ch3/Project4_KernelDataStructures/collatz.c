#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("112590024");
MODULE_DESCRIPTION("Project 4 Part II - Collatz Sequence Generator with Linked List");
MODULE_VERSION("1.0");

/* Module parameter for initial value */
static int initial_value = 10;
module_param(initial_value, int, S_IRUGO);
MODULE_PARM_DESC(initial_value, "Initial value for Collatz sequence (default: 10)");

/* Structure to store a single element in the sequence */
struct collatz_node {
    unsigned long value;
    struct list_head list;
};

/* Head of the linked list */
static LIST_HEAD(collatz_list);

/* Function to generate and store the Collatz sequence */
static int generate_collatz_sequence(unsigned long n)
{
    struct collatz_node *node;
    int count = 0;

    if (n <= 0) {
        printk(KERN_ERR "Collatz: Initial value must be positive\n");
        return -EINVAL;
    }

    printk(KERN_INFO "=== Generating Collatz Sequence for n = %lu ===\n", n);

    while (n != 1) {
        /* Allocate memory for new node */
        node = kmalloc(sizeof(struct collatz_node), GFP_KERNEL);
        if (!node) {
            printk(KERN_ERR "Collatz: Failed to allocate memory for node\n");
            return -ENOMEM;
        }

        /* Store current value */
        node->value = n;
        list_add_tail(&node->list, &collatz_list);
        count++;

        /* Apply Collatz rule */
        if (n % 2 == 0) {
            n = n / 2;
        } else {
            n = 3 * n + 1;
        }
    }

    /* Add the final value (1) */
    node = kmalloc(sizeof(struct collatz_node), GFP_KERNEL);
    if (!node) {
        printk(KERN_ERR "Collatz: Failed to allocate memory for final node\n");
        return -ENOMEM;
    }
    node->value = 1;
    list_add_tail(&node->list, &collatz_list);
    count++;

    return count;
}

/* Module initialization function */
static int __init collatz_init(void)
{
    struct collatz_node *entry;
    int count;
    int i = 0;

    printk(KERN_INFO "=== Collatz Sequence Module Loaded ===\n");

    /* Generate the Collatz sequence */
    count = generate_collatz_sequence(initial_value);
    if (count < 0) {
        return count;
    }

    /* Traverse and display the sequence */
    printk(KERN_INFO "\n--- Traversing Collatz Sequence ---\n");
    list_for_each_entry(entry, &collatz_list, list) {
        i++;
        printk(KERN_INFO "Step %d: %lu\n", i, entry->value);
    }
    printk(KERN_INFO "--- Total steps in sequence: %d ---\n\n", count);

    return 0;
}

/* Module exit function */
static void __exit collatz_exit(void)
{
    struct collatz_node *entry, *temp;

    printk(KERN_INFO "\n=== Collatz Sequence Module Unloading ===\n");
    printk(KERN_INFO "--- Removing all sequence elements ---\n");

    /* Remove all elements from the list and free memory */
    list_for_each_entry_safe(entry, temp, &collatz_list, list) {
        list_del(&entry->list);
        kfree(entry);
    }

    printk(KERN_INFO "--- All elements removed, memory freed ---\n");
    printk(KERN_INFO "=== Collatz Sequence Module Unloaded ===\n\n");
}

module_init(collatz_init);
module_exit(collatz_exit);