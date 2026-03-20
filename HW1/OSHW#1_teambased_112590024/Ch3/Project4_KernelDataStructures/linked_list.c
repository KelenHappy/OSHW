#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("112590024");
MODULE_DESCRIPTION("Project 4 Part I - Linked List with Color Elements");

/* Define the color structure */
struct color {
    int id;
    char name[50];
    int rgb[3];  /* Red, Green, Blue values */
    struct list_head list;  /* Kernel list node */
};

/* Declare the head of the linked list */
static LIST_HEAD(color_list);

/* Module init function */
static int __init linked_list_init(void)
{
    struct color *red, *green, *blue, *yellow;

    printk(KERN_INFO "=== Linked List Module Loaded ===\n");

    /* Create and initialize the first color (Red) */
    red = kmalloc(sizeof(struct color), GFP_KERNEL);
    if (!red) {
        printk(KERN_ERR "Failed to allocate memory for red\n");
        return -ENOMEM;
    }
    red->id = 1;
    strcpy(red->name, "Red");
    red->rgb[0] = 255;
    red->rgb[1] = 0;
    red->rgb[2] = 0;
    list_add_tail(&red->list, &color_list);

    /* Create and initialize the second color (Green) */
    green = kmalloc(sizeof(struct color), GFP_KERNEL);
    if (!green) {
        printk(KERN_ERR "Failed to allocate memory for green\n");
        kfree(red);
        return -ENOMEM;
    }
    green->id = 2;
    strcpy(green->name, "Green");
    green->rgb[0] = 0;
    green->rgb[1] = 255;
    green->rgb[2] = 0;
    list_add_tail(&green->list, &color_list);

    /* Create and initialize the third color (Blue) */
    blue = kmalloc(sizeof(struct color), GFP_KERNEL);
    if (!blue) {
        printk(KERN_ERR "Failed to allocate memory for blue\n");
        kfree(red);
        kfree(green);
        return -ENOMEM;
    }
    blue->id = 3;
    strcpy(blue->name, "Blue");
    blue->rgb[0] = 0;
    blue->rgb[1] = 0;
    blue->rgb[2] = 255;
    list_add_tail(&blue->list, &color_list);

    /* Create and initialize the fourth color (Yellow) */
    yellow = kmalloc(sizeof(struct color), GFP_KERNEL);
    if (!yellow) {
        printk(KERN_ERR "Failed to allocate memory for yellow\n");
        kfree(red);
        kfree(green);
        kfree(blue);
        return -ENOMEM;
    }
    yellow->id = 4;
    strcpy(yellow->name, "Yellow");
    yellow->rgb[0] = 255;
    yellow->rgb[1] = 255;
    yellow->rgb[2] = 0;
    list_add_tail(&yellow->list, &color_list);

    printk(KERN_INFO "\n--- Traversing Linked List ---\n");

    /* Traverse the linked list and print its contents */
    struct color *entry;
    list_for_each_entry(entry, &color_list, list) {
        printk(KERN_INFO "Color ID: %d, Name: %s, RGB: (%d, %d, %d)\n",
               entry->id, entry->name, entry->rgb[0], entry->rgb[1], entry->rgb[2]);
    }

    printk(KERN_INFO "--- End of List Traversal ---\n\n");

    return 0;
}

/* Module exit function */
static void __exit linked_list_exit(void)
{
    struct color *entry, *temp;

    printk(KERN_INFO "\n=== Linked List Module Unloading ===\n");
    printk(KERN_INFO "--- Removing all elements from the list ---\n");

    /* Traverse the list and remove all elements */
    list_for_each_entry_safe(entry, temp, &color_list, list) {
        printk(KERN_INFO "Removing: ID %d, Name: %s\n", entry->id, entry->name);
        list_del(&entry->list);
        kfree(entry);
    }

    printk(KERN_INFO "--- All elements removed, memory freed ---\n");
    printk(KERN_INFO "=== Linked List Module Unloaded ===\n\n");
}

module_init(linked_list_init);
module_exit(linked_list_exit);