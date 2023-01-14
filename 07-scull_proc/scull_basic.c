#include "scull.h"

/* user input parameters */
module_param(scull_nr_devs, int, S_IRUGO);
module_param(scull_quantum, int, S_IRUGO);
module_param(scull_qset, int, S_IRUGO);

/* scull device essential property */
static dev_t scull_dev_num;
static struct scull_dev *scull_devs;

static const struct file_operations scull_fops = {
    .owner  = THIS_MODULE,
    .read   = scull_read,
    .write  = scull_write,
    .open   = scull_open,
    .release = scull_release
};


#if SCULL_DEBUG

/**
 * Here are our sequence iteration methods.  Our "position" is
 * simply the device number.
 */

static void * scull_seq_start (struct seq_file *m, loff_t *pos) {
    /** 
     * The position is often interpreted as a cursor pointing 
     * to the next item in the sequence. The scull driver 
     * interprets each device as one item in the sequence.
     */
    if (*pos >= scull_nr_devs) {
        return 0; /* No more data */
    }
    return scull_devs + *pos; /* update new position */
}

static void scull_seq_stop (struct seq_file *m, void *v) {
    /* Left empty */
}

static void * scull_seq_next (struct seq_file *m, void *v, loff_t *pos) {
    /** 
     * 'next' should increment the value pointed to by pos;
     * depending on how your iterator works. 
     */
    (*pos)++;
    if (*pos >= scull_nr_devs) {
        return NULL;
    }
    return scull_devs + *pos; /* update new position */
}

static int scull_seq_show (struct seq_file *m, void *v) {
    struct scull_dev *dev = (struct scull_dev *) v;
    struct scull_qset *d;
    int i;

    if (mutex_lock_interruptible(&dev->mlock)) {
        return -ERESTARTSYS;
    }

    seq_printf(m, "Device %i: qset %i, q %i, sz %li\n", 
        (int)(dev - scull_devs), dev->qset, dev->quantum, dev->size);

    for (d = dev->data; d; d = d->next) {
        /* print the addresses of qset item and start qset */
        seq_printf(m, " item at %p, qset at %p\n", d, d->data);
        if (d->data && !d->next) { /* scan the list */
            for (i = 0; i < dev->qset; i++) {
                if (d->data[i]) {
                    seq_printf(m, "    % 4i: %8p\n", i, d->data[i]);
                }
            }
        }
    }

    mutex_unlock(&dev->mlock);
    return 0;
}

struct seq_operations scull_seq_ops = {
    .start = scull_seq_start,
    .stop  = scull_seq_stop,
    .next  = scull_seq_next,
    .show  = scull_seq_show
};

static int scull_proc_open(struct inode *inode, struct file *file) {
    return seq_open(file, &scull_seq_ops);
}

static struct proc_ops scull_proc_ops = {
    .proc_open    = scull_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = seq_release
};


// Actually create (and remove) the /proc file(s).
static void scull_create_proc(void) {
    proc_create("scull_seq", 0, NULL, &scull_proc_ops);
}

static void scull_remove_proc(void)
{
    /* no problem if it was not registered */
    remove_proc_entry("scull_seq", NULL);
}

#endif


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
#ifdef SCULL_DEBUG /* only when debugging */
    scull_create_proc();
#endif
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
#ifdef SCULL_DEBUG /* only when debugging */
    scull_remove_proc();
#endif
    pr_info("scull module cleanup \n");
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module creates scull device with /proc subsystem");
MODULE_INFO(modparams, "07-scull_proc");