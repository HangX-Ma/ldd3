/**
 * @file completion.c
 * @author HangX-Ma
 * @brief Completions are a lightweight mechanism with one task: 
 * allowing one thread to tell another that the job is done.
 * @version 0.1
 * @date 2023-01-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/types.h>


/* format the print function */
#undef pr_fmt
#define pr_fmt(fmt) "%s " fmt, __func__

#define COMPLETION_NR_DEVS  3
#define COMPLETION_NAME     "completion"


static dev_t completion_dev_num;
struct completion_t {
	struct completion completion;
    struct cdev cdev;
};

struct completion_t *comp_devices;

static ssize_t completion_read (struct file *filp, 
                                char __user *buf, 
                                size_t count, 
                                loff_t *fpos) {
    int cpu;
    struct completion_t *comp_dev = (struct completion_t *)filp->private_data;

    cpu = get_cpu();
    pr_notice("process %i (%s) is going to sleep on cpu %d\n", current->pid, current->comm, cpu);
    put_cpu();
    wait_for_completion(&comp_dev->completion);
    cpu = get_cpu();
    pr_notice("process %i (%s) is waken up on cpu %d\n", current->pid, current->comm, cpu);
    put_cpu();

    return 0;
}

static ssize_t completion_write (struct file *filp, 
                                const char __user *buf, 
                                size_t count, 
                                loff_t *fpos) {
    struct completion_t *comp_dev = (struct completion_t *)filp->private_data;

    int cpu = get_cpu();
    pr_notice("process %i (%s) awakening the readers on cpu %d\n", current->pid, current->comm, cpu);
    put_cpu();

    complete(&comp_dev->completion);

    return count;
}

static int completion_open (struct inode *inode, struct file *filp) {
    struct completion_t* comp_dev;

    pr_notice("is invoked. Allocate private_data in 'filp'. \n");
    comp_dev = container_of(inode->i_cdev, struct completion_t, cdev);
    /* acquire acquire dev information */
    filp->private_data = comp_dev;

    return 0;
}

static int completion_release (struct inode *inode, struct file *filp) {
    pr_notice("is invoked\n");

    return 0;
}
 

static struct file_operations completion_fops = {
    .owner = THIS_MODULE,
    .read = completion_read,
    .write = completion_write,
    .open  = completion_open,
    .release = completion_release,
};


static int __init completion_init(void) {
    int ret, i;

    pr_info("- Completion module is loaded \n");
    ret = alloc_chrdev_region(&completion_dev_num, 0, 
                        COMPLETION_NR_DEVS,
                        COMPLETION_NAME);
    if (ret < 0) {
        pr_err("Allocate chrdev failed.\n");
        goto out;
    }

    /* dynamically allocate memory for scull_devs array */
    comp_devices = kzalloc(COMPLETION_NR_DEVS * sizeof(struct completion_t), GFP_KERNEL);
    if (!comp_devices) {
        ret = -ENOMEM;
        pr_err("Allocate device structure memory failed\n");
        goto unreg_chrdev;
    }

    /* initialize the completion devices */
    pr_info("- Initialize the completion devices\n");
    for (i = 0; i < COMPLETION_NR_DEVS; i++) {
        cdev_init(&comp_devices[i].cdev, &completion_fops);
        init_completion(&comp_devices[i].completion);
        ret = cdev_add(&comp_devices[i].cdev, completion_dev_num, 1);
        if (ret) {
            pr_err("Error %d adding scull%d\n", ret, i);
            goto unreg_cdev;
        }
    }

    return 0;

unreg_cdev:
    if (comp_devices) {
        for (i = 0; i < COMPLETION_NR_DEVS; i++) {
            cdev_del(&comp_devices[i].cdev);
        }
        kfree(comp_devices);
    }
unreg_chrdev:
    unregister_chrdev_region(completion_dev_num, COMPLETION_NR_DEVS);
out:
    pr_info("- Module insertion failed \n");
    return ret;
}


static void __exit completion_exit(void) {
    int i;
    if (comp_devices) {
        for (i = 0; i < COMPLETION_NR_DEVS; i++) {
            cdev_del(&comp_devices[i].cdev);
        }
        kfree(comp_devices);
    }
    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(completion_dev_num, COMPLETION_NR_DEVS);
    pr_info("- Completion module cleanup \n");
}

module_init(completion_init);
module_exit(completion_exit);

MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module creates device using completion interface");
MODULE_INFO(modparams, "08-completion");