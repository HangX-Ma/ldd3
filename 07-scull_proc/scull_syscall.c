#include "scull.h"

int scull_nr_devs = SCULL_NR_DEVS;
int scull_quantum = SCULL_QUANTUM;
int scull_qset    = SCULL_QSET;

/**
 * Allocate memory and link each struct_qest as a list
 */
struct scull_qset *scull_follow(struct scull_dev *dev, int item) {
    struct scull_qset *qs_data = dev->data;
    
    /* allocate 'scull_qset' structure for 'scull_dev' container */
    if (!qs_data) {
        qs_data = kzalloc(sizeof(struct scull_qset), GFP_KERNEL);
        if (qs_data == NULL) {
            return NULL; /* Never mind */
        }
    }

    /* continue to allocate memory for qset and link them as a list */
    while (item--) {
        if (!qs_data->next) {
            qs_data->next = kzalloc(sizeof(struct scull_qset), GFP_KERNEL);
            if (qs_data->next == NULL) {
                return NULL; /* Never mind */
            }
        }
        qs_data = qs_data->next; /* update list header */
    }

    return qs_data;
}

#if 1

ssize_t scull_read (struct file *filp, char __user *buf, size_t count, loff_t *fpos) {
    struct scull_dev *dev = filp->private_data;
    struct scull_qset *dptr;

    int quantum  = dev->quantum;
    int qset     = dev->qset;
    int itemsize = quantum * qset; /* total bytes */

    int item, remained, qblock, qoffset;
    ssize_t retval = 0;

    pr_info("is invoked\n");

    if (mutex_lock_interruptible(&dev->mlock)) {
        return -ERESTARTSYS;
    }

    /* check bound limitation */
    if (*fpos >= dev->size) {
        goto out;
    }
    if (*fpos + count > dev->size) {
        count = dev->size - *fpos;
    }

    /* find listitem, qset index, and offset in the quantum */
    item     = (long)*fpos / itemsize;
    remained = (long)*fpos % itemsize;
    qblock = remained / quantum; 
    qoffset = remained % quantum;

    /* follow the list up to the right position (defined elsewhere) */
    dptr = scull_follow(dev, item);

    if (dptr == NULL || !dptr->data || ! dptr->data[qblock])
        goto out; /* don't fill holes */

    /* read only up to the end of this quantum */
    if (count > quantum - qoffset) {
        count = quantum - qoffset;
    }

    if (copy_to_user(buf, dptr->data[qblock] + qoffset, count)) {
        retval = -EFAULT;
        goto out;
    }
    *fpos += count;
    retval = count;

out:
    pr_info("RD pos = %lld, block = %d, offset = %d, read %lu bytes\n",
            *fpos, qblock, qoffset, count);
    mutex_unlock(&dev->mlock);
    return retval;
}

ssize_t scull_write (struct file *filp, const char __user *buf, size_t count, loff_t *fpos) {
    struct scull_dev *dev = filp->private_data;
    struct scull_qset *dptr;

    int quantum  = dev->quantum;
    int qset     = dev->qset;
    int itemsize = quantum * qset; /* total bytes */

    int item, remained, qblock, qoffset;
    ssize_t retval = -ENOMEM;

    pr_info("is invoked\n");

    if (mutex_lock_interruptible(&dev->mlock)) {
        return -ERESTARTSYS;
    }

    /* find listitem, qset index, and offset in the quantum */
    item     = (long)*fpos / itemsize;
    remained = (long)*fpos % itemsize;
    qblock = remained / quantum; 
    qoffset = remained % quantum;

    /* follow the list up to the right position (defined elsewhere) */
    dptr = scull_follow(dev, item);

    if (dptr == NULL) {
        goto out;
    }
    if (!dptr->data) {
        dptr->data = kzalloc(qset * sizeof(char *), GFP_KERNEL);
        if (!dptr->data) {
            goto out;
        }
    }
    if (!dptr->data[qblock]) {
        dptr->data[qblock] = kmalloc(quantum, GFP_KERNEL);
        if (!dptr->data[qblock])
            goto out;
    }

    /* read only up to the end of this quantum */
    if (count > quantum - qoffset) {
        count = quantum - qoffset;
    }

    if (copy_from_user(dptr->data[qblock] + qoffset, buf, count)) {
        retval = -EFAULT;
        goto out;
    }
    *fpos += count;
    retval = count;

    /* update the size */
    if (dev->size < *fpos) {
        dev->size = *fpos;
    }

out:
    pr_info("WR pos = %lld, block = %d, offset = %d, read %lu bytes\n",
            *fpos, qblock, qoffset, count);
    mutex_unlock(&dev->mlock);
    return retval;
}


int scull_open (struct inode *inode, struct file *filp) {
    struct scull_dev *dev; /* device information */

    pr_info("is invoked\n");

    dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev; /* acquire information */

    /* now trim to 0 the length of the device if open was write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (mutex_lock_interruptible(&dev->mlock)) {
            return -ERESTARTSYS;
        }
        scull_trim(dev); /* ignore errors */
        mutex_unlock(&dev->mlock);
    }

    return 0;
}
#endif

int scull_release (struct inode *inode, struct file *filp) {
    pr_info("scull_release minor=%u\n", MINOR(inode->i_rdev));

    return 0;
}

/**
 * Empty out the scull device; must be called with 
 * the device semaphore held.
 */
int scull_trim(struct scull_dev *dev) {
    int i;
    struct scull_qset *next = NULL;
    struct scull_qset *dptr = NULL;
    int qset = dev->qset;

    for (dptr = dev->data; dptr; dptr = next) {
        if (dptr->data) {
            for (i = 0; i < qset; i++) {
                kfree(dptr->data[i]);
            }
            kfree(dptr->data);
            dptr->data = NULL;
        }
        next = dptr->next;
        kfree(dptr);
    }
    dev->qset    = scull_qset;
    dev->quantum = scull_quantum;
    dev->size    = 0;
    dev->data    = NULL;

    return 0;
}
