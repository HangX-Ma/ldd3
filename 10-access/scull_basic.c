#include "scull.h"

/* user input parameters */
module_param(scull_nr_devs, int, S_IRUGO);
module_param(scull_quantum, int, S_IRUGO);
module_param(scull_qset, int, S_IRUGO);

/* scull device essential property */
static dev_t scull_dev_num;
static struct scull_dev *scull_devs;

static const struct file_operations scull_fops = {
    .owner   = THIS_MODULE,
    .read    = scull_read,
    .write   = scull_write,
    .open    = scull_open,
    .llseek  = scull_llseek,
    .release = scull_release,
};


static int __init scull_init(void) {
    int ret, i;

    pr_info("Initialize scull module, scull_quantum: %d, scull_qset: %d, scull_nr_devs: %d\n", 
            scull_quantum, scull_qset, scull_nr_devs);

    /* request dynamicly-allocated device numbers */
    ret = alloc_chrdev_region(&scull_dev_num, 0,    /* Base number */
                            scull_nr_devs,          /* Total device number */
                            SCULL_MODULE_NAME       /* Device module name */
                            );
    if (ret < 0) {
        pr_err("Allocate chrdev failed.\n");
        goto out;
    }

    /* dynamically allocate memory for scull_devs array */
    scull_devs = kzalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);
    if (!scull_devs) {
        ret = -ENOMEM;
        pr_err("Allocate device structure memory failed\n");
        goto unreg_chrdev;
    }

    /* initialize the scull devices */
    pr_info("Initialize the scull devices\n");
    for (i = 0; i < scull_nr_devs; i++) {
        cdev_init(&scull_devs[i].cdev, &scull_fops);
        scull_devs[i].cdev.owner = THIS_MODULE;
        scull_devs[i].quantum    = scull_quantum;
        scull_devs[i].qset       = scull_qset;
        scull_devs[i].size       = 0;
        scull_devs[i].data       = NULL;
        mutex_init(&scull_devs[i].mlock);
        ret = cdev_add(&scull_devs[i].cdev, 
                        MKDEV(MAJOR(scull_dev_num), MINOR(scull_dev_num) + i), /* base responsible device number */
                        1 /* the number of consecutive minor numbers corresponding to this device */);
        if (ret) {
            pr_err("Error %d adding scull%d\n", ret, i);
            goto unreg_cdev;
        }
    }
    pr_info("Module init was successful\n");
    return 0;

unreg_cdev:
    if (scull_devs) {
        for (i = 0; i < scull_dev_num; i++) {
            scull_trim(scull_devs + i);
            cdev_del(&scull_devs[i].cdev);
        }
        kfree(scull_devs);
    }
unreg_chrdev:
    unregister_chrdev_region(scull_dev_num, scull_nr_devs);
out:
    pr_info("Module insertion failed \n");
    return ret;
}

static void __exit scull_exit(void) {
    int i;
    if (scull_devs) {
        for (i = 0; i < scull_nr_devs; i++) {
            scull_trim(scull_devs + i);
            cdev_del(&scull_devs[i].cdev);
        }
        kfree(scull_devs);
    }
    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(scull_dev_num, scull_nr_devs);
    pr_info("scull module clean up \n");
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module creates scull device");
MODULE_INFO(modparams, "10-access");