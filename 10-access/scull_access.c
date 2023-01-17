#include "scull.h"

/**
 * The first is scull Single-Open device, which has an
 * hw structure and an open count.
 */



static struct scull_dev scull_s_device;
static atomic_t scull_s_available = ATOMIC_INIT(1);

static int scull_s_open(struct inode *inode, struct file *filp) {
    struct scull_dev *dev = &scull_s_device; /* device information */

    if (!atomic_dec_and_test(&scull_s_available)) {
        atomic_inc(&scull_s_available);
        return -EBUSY; /* available */
    }
    /* copy everything from the bare device */
    if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
        scull_trim(dev);
    }
    filp->private_data = dev;

    return 0;          /* success */
}

static int scull_s_release(struct inode *inode, struct file *filp) {
    atomic_inc(&scull_s_available); /* release the device */

    return 0;
}

struct file_operations scull_single_fops = {
    .owner =	THIS_MODULE,
    .llseek =     	scull_llseek,
    .read =       	scull_read,
    .write =      	scull_write,
    .open =       	scull_s_open,
    .release =    	scull_s_release,
};

/**
 * Next, the "uid" device. It can be opened multiple times by the
 * same user, but access is denied to other users if the device is open
 */
static struct scull_dev scull_u_device;
static int scull_u_count;   /* initialized to 0 by default */
static uid_t scull_u_owner; /* initialized to 0 by default */
static DEFINE_SPINLOCK(scull_u_lock);

static int scull_u_open(struct inode *inode, struct file *filp) {
    struct scull_dev *dev = &scull_u_device; /* device information */

    /* check the uid and euid of the user */
    spin_lock(&scull_u_lock);
    if (scull_u_count &&
            (scull_u_owner != current_uid().val) &&  /* allow user */
            (scull_u_owner != current_euid().val) && /* allow whoever did su */
            !capable(CAP_DAC_OVERRIDE)) {            /* still allow root */
        spin_unlock(&scull_u_lock);
        return -EBUSY;   /* -EPERM would confuse the user */
    }
    if (scull_u_count == 0) {
        scull_u_owner = current_uid().val; /* grab it */
    }
    scull_u_count++;
    spin_unlock(&scull_u_lock);

    /* copy everything from the bare device */
    if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
        scull_trim(dev);
    }
    filp->private_data = dev;

    return 0;          /* success */
}

static int scull_u_release(struct inode *inode, struct file *filp) {
    spin_lock(&scull_u_lock);
    scull_u_count--; /* nothing else */
    spin_unlock(&scull_u_lock);
    return 0;
}


struct file_operations scull_user_fops = {
    .owner =      THIS_MODULE,
    .llseek =     scull_llseek,
    .read =       scull_read,
    .write =      scull_write,
    .open =       scull_u_open,
    .release =    scull_u_release,
};

/**
 * Next, the device with blocking-open based on uid
 */

static struct scull_dev scull_w_device;
static int scull_w_count;   /* initialized to 0 by default */
static uid_t scull_w_owner; /* initialized to 0 by default */
static DECLARE_WAIT_QUEUE_HEAD(scull_w_wait);
static DEFINE_SPINLOCK(scull_w_lock);

static inline int scull_w_available(void)
{
    return scull_w_count == 0 ||
        scull_w_owner == current_uid().val ||
        scull_w_owner == current_euid().val ||
        capable(CAP_DAC_OVERRIDE);
}


static int scull_w_open(struct inode *inode, struct file *filp)
{
    struct scull_dev *dev = &scull_w_device; /* device information */

    spin_lock(&scull_w_lock);
    while (! scull_w_available()) {
        spin_unlock(&scull_w_lock);
        if (filp->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }
        if (wait_event_interruptible (scull_w_wait, scull_w_available())) {
            return -ERESTARTSYS; /* tell the fs layer to handle it */
        }
        spin_lock(&scull_w_lock);
    }
    if (scull_w_count == 0) {
        scull_w_owner = current_uid().val; /* grab it */
    }
    scull_w_count++;
    spin_unlock(&scull_w_lock);

    /* then, everything else is copied from the bare scull device */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        scull_trim(dev);
    }
    filp->private_data = dev;
    return 0;          /* success */
}

static int scull_w_release(struct inode *inode, struct file *filp)
{
    int temp;

    spin_lock(&scull_w_lock);
    scull_w_count--;
    temp = scull_w_count;
    spin_unlock(&scull_w_lock);

    if (temp == 0) {
        /* awake other uid's */
        wake_up_interruptible_sync(&scull_w_wait);
    }
    return 0;
}


struct file_operations scull_wusr_fops = {
    .owner =      THIS_MODULE,
    .llseek =     scull_llseek,
    .read =       scull_read,
    .write =      scull_write,
    .open =       scull_w_open,
    .release =    scull_w_release,
};
