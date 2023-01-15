/**
 * @file scull_pipe.c
 * @author HangX-Ma
 * @brief scullpipe device implements blocking I/O.
 * @version 0.1
 * @date 2023-01-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "scull.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s " fmt, __func__

/* parameters */
static int scull_p_nr_devs = SCULL_P_NR_DEVS;   /* number of pipe devices */
static int scull_p_buffer  =  SCULL_P_BUFFER;   /* buffer size */
static dev_t scull_p_dev_num;

module_param(scull_p_nr_devs, int, 0);
module_param(scull_p_buffer, int, 0);

static int scull_p_fasync(int fd, struct file *filp, int mode);
static int spacefree(struct scull_pipe *dev);
static int scull_getwritespace(struct scull_pipe *dev, struct file *filp);

static struct scull_pipe *scull_p_devices;
static dev_t scull_p_dev_num;

static int 
scull_p_open(struct inode *inode, struct file *filp) {
    struct scull_pipe *dev;
    /* store scull_pipe device datum */
    dev = container_of(inode->i_cdev, struct scull_pipe, cdev);
    filp->private_data = dev;

    /* memory allocation and critical datum access needs mutex lock protection */
    if (mutex_lock_interruptible(&scull_p_devices->mlock)) {
        return -ERESTARTSYS;
    }

    /* allocate memory */
    if (!dev->buffer) {
        dev->buffer = kzalloc(scull_p_buffer, GFP_KERNEL);
        if (!dev->buffer) {
            mutex_unlock(&scull_p_devices->mlock);
            return -ENOMEM; /* failed */
        }
    }
    dev->end        = dev->buffer + scull_p_buffer;
    dev->buffersize = scull_p_buffer;
    dev->rp = dev->wp = dev->buffer;

    /** 
     * struct file provide f_mode to recognize the WR/RD right
     * of one file, rather than f_flag (included in <linux/fcntl.h>).
     */
    if (filp->f_mode & FMODE_READ) {
        dev->nreaders++;
    }
    if (filp->f_mode & FMODE_WRITE) {
        dev->nwriters++;
    }
    mutex_unlock(&dev->mlock);
    /**
     * most devices offer a data flow rather than a data area,
     * and seeking those devices does not make sense.
     */
    return nonseekable_open(inode, filp);
}


/**
 * The read implementation manages both blocking and nonblocking input 
 */
static ssize_t 
scull_p_read (struct file *filp, char __user *buf, size_t count, loff_t *fpos) {
    struct scull_pipe *dev = filp->private_data;

    /** 
     * No crucial time demand and mutex lock leading to process sleep
     * if no lock acquired, which renders mutex lock is quite suitable
     * for data reading.
     */
    if (mutex_lock_interruptible(&dev->mlock)) {
        return -ERESTARTSYS;
    }

    while (dev->rp == dev->wp) { /* nothing left to read */
        /**
         * Release mutex lock avoid deadlock in sleeping with lock holden,
         * Otherwise, writer would ever have the opportunity to wake us up.
         */
        mutex_unlock(&dev->mlock);
        /* NONBLOCK: repeat syscall for another time */
        if (filp->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }
        pr_debug("- %s reading: going to sleep\n", current->comm);
        /* mutex lock needs interruptable feature to waken this process */
        if (wait_event_interruptible(dev->inq, (dev->rp != dev->wp))) {
            return -ERESTARTSYS; /* signal fd layer to process it */
        }
        /** 
         * However, even in the absence of a signal, we do not yet know for sure 
         * that there is data there for the taking. Somebody else could have been
         * waiting for data as well, and they might win the race and get the data first.
         */
        if (mutex_lock_interruptible(&dev->mlock)) {
            return -ERESTARTSYS;
        }
    }
    /* ---------------------- data arrives the device ---------------------- */
    if (dev->wp > dev->rp) {
        count = min(count, (size_t)(dev->wp - dev->rp));
    }
    else {
        count = min(count, (size_t)(dev->end - dev->rp));
    }

    if (copy_to_user(buf, dev->rp, count)) {
        mutex_unlock(&dev->mlock);
        return -EFAULT;
    }
    dev->rp += count;
    if (dev->rp == dev->wp) {
        dev->rp = dev->buffer; /* wrapped */ 
    }
    mutex_unlock(&dev->mlock);
    /* finally, awake any writers and return */
    wake_up_interruptible(&dev->outq);
    pr_debug("- %s did read %li bytes\n",current->comm, (long)count);

    return count;
}


static ssize_t 
scull_p_write (struct file *filp, const char __user *buf, size_t count, loff_t *fpos) {
    struct scull_pipe *dev = filp->private_data;
    int result;

    if (mutex_lock_interruptible(&dev->mlock)) {
        return -ERESTARTSYS;
    }

    /* Make sure there's space to write */
    result = scull_getwritespace(dev, filp);
    if (result) {
        return result; /* scull_getwritespace has called mutex_unlock(&dev->sem) */
    }
    /* sleeping if need be until that space comes available. */
    count = min(count, (size_t)spacefree(dev)); // update count first 
    if (dev->wp >= dev->rp) {
        count = min(count, (size_t)(dev->end - dev->wp));
    } /* to end-of-buf */
    else {
        count = min(count, (size_t)(dev->rp - dev->wp - 1));
    } /* the write pointer has wrapped, fill up to rp-1 */
    pr_info("Going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);
    if (copy_from_user(dev->wp, buf, count)) {
        mutex_unlock(&dev->mlock);
        return -EFAULT;
    }
    dev->wp += count;
    if (dev->wp == dev->end) {
        dev->wp = dev->buffer; /* wrapped */
    }
    mutex_unlock(&dev->mlock);
    /* finally, awake any reader */
    wake_up_interruptible(&dev->inq);  /* blocked in read() and select() */
    if (dev->async_queue) {
        kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
    }
    pr_debug("- %s did write %li bytes\n",current->comm, (long)count);

    return count;
}

static unsigned int scull_p_poll(struct file *filp, poll_table *wait) {
    struct scull_pipe *dev = filp->private_data;
    unsigned int mask = 0;

    mutex_lock(&dev->mlock);
    poll_wait(filp, &dev->inq,  wait);
    poll_wait(filp, &dev->outq, wait);
    if (dev->rp != dev->wp) {
        mask |= POLLIN | POLLRDNORM;    /* readable */
    }
    if (spacefree(dev)) {
        mask |= POLLOUT | POLLWRNORM;   /* writable */
    }
    mutex_unlock(&dev->mlock);
    return mask;
}


static int scull_p_fasync(int fd, struct file *filp, int mode) {
    struct scull_pipe *dev = filp->private_data;

    return fasync_helper(fd, filp, mode, &dev->async_queue);
}


static int 
scull_p_release (struct inode *inode, struct file *filp) {
    struct scull_pipe *dev = filp->private_data;

    /* remove this filp from the asynchronously notified filp's */
    scull_p_fasync(-1, filp, 0);
    mutex_lock(&dev->mlock);
    if (filp->f_mode & FMODE_READ) {
        dev->nreaders--;
    }
    if (filp->f_mode & FMODE_WRITE) {
        dev->nwriters--;
    }
    /* no reader and writer reference means the final close */
    if (dev->nreaders + dev->nwriters == 0) {
        kfree(dev->buffer);
        dev->buffer = NULL; /* the other fields are not checked on open */
    }
    mutex_unlock(&dev->mlock);

    return 0;
}

static int 
spacefree(struct scull_pipe *dev) {
    /* left one buffer space for rp and wp recognition */
    if (dev->rp == dev->wp) {
        return dev->buffersize - 1;
    }
    /* circular buffer needs overflow protection */
    return (dev->rp + dev->buffersize - dev->wp) % dev->buffersize - 1;
}

/**
 * Wait for space for writing; caller must hold device semaphore. 
 * On error the semaphore will be released before returning. 
 */
static int scull_getwritespace(struct scull_pipe *dev, struct file *filp) {
    while (spacefree(dev) == 0) {
        DEFINE_WAIT(wait);

        mutex_unlock(&dev->mlock);
        if (filp->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }
        pr_debug("- %s writing: going to sleep\n", current->comm);
        prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
        /**
         * The call to signal_pending tells us whether we were awakened 
         * by a signal; if so, we need to return to the user and let 
         * them try again later.
         */
        if (spacefree(dev) == 0) {
            schedule();
        }
        finish_wait(&dev->outq, &wait);
        if (signal_pending(current)) {
            return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
        }
        /* acquire mutex lock again for next checking loop */
        if (mutex_lock_interruptible(&dev->mlock)) {
            return -ERESTARTSYS;
        }
    }
    return 0;
}


struct file_operations scull_pipe_fops = {
    .owner =	THIS_MODULE,
    .llseek =	no_llseek,
    .read =		scull_p_read,
    .write =	scull_p_write,
    .poll =		scull_p_poll,
    .open =		scull_p_open,
    .release =	scull_p_release,
    .fasync =	scull_p_fasync,
};


static int __init scull_p_init(void) {
    int ret, i;

    pr_info("- scullpipe module is loaded\n");
    ret = alloc_chrdev_region(&scull_p_dev_num, 0, scull_p_nr_devs, "scullpipe");
    if (ret < 0) {
        pr_err("Allocate chrdev failed.\n");
        goto out;
    }
    
    scull_p_devices = kzalloc(scull_p_nr_devs * sizeof(struct scull_pipe), GFP_KERNEL);
    if (!scull_p_devices) {
        ret = -ENOMEM;
        pr_err("Allocate device structure memory failed\n");
        goto unreg_chrdev;
    }

    for (i = 0; i < scull_p_nr_devs; i++) {
        init_waitqueue_head(&scull_p_devices[i].inq);
        init_waitqueue_head(&scull_p_devices[i].outq);
        mutex_init(&scull_p_devices[i].mlock);
	    cdev_init(&scull_p_devices[i].cdev, &scull_pipe_fops);
        scull_p_devices[i].cdev.owner = THIS_MODULE;
        if (ret) {
            pr_err("Error %d adding scull%d\n", ret, i);
            goto unreg_cdev;
        }
    }

    return 0;
unreg_cdev:
    if (scull_p_devices) {
        for (i = 0; i < scull_p_nr_devs; i++) {
            cdev_del(&scull_p_devices[i].cdev);
        }
        kfree(scull_p_devices);
    }
unreg_chrdev:
    unregister_chrdev_region(scull_p_dev_num, scull_p_nr_devs);
out:
    pr_err("- Module insertion failed\n");
    return ret;
}

static void __exit scull_p_exit(void) {
    int i;
	if (!scull_p_devices)
		return; /* nothing else to release */

	for (i = 0; i < scull_p_nr_devs; i++) {
		cdev_del(&scull_p_devices[i].cdev);
		kfree(scull_p_devices[i].buffer);
	}
	kfree(scull_p_devices);
	unregister_chrdev_region(scull_p_dev_num, scull_p_nr_devs);
	scull_p_devices = NULL; /* pedantic */
}


module_init(scull_p_init);
module_exit(scull_p_exit);

MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A scull module with pipe and circular buffer");
MODULE_INFO(modparams, "09-scull_pipe");