#include "char_device.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__

static struct class *char_class;
static struct cdev char_cdev[DUMMY_CHAR_NBANK]; // character device
static dev_t char_dev_num; // device number


loff_t char_llseek (struct file *filp, loff_t offset, int whence) {
    return filp->f_pos;
}

ssize_t char_read (struct file *filp, char __user *buffer, size_t count, loff_t *fpos) {
    return 0;
}

ssize_t char_write (struct file *filp, const char __user *buffer, size_t count, loff_t *fpos) {
    return count;
}

int char_open (struct inode *inode, struct file *filp) {
    return 0;
}

int char_release (struct inode *inode, struct file *filp) {
    return 0;
}


struct file_operations fops = {
    .open    = char_open,
    .read    = char_read,
    .write   = char_write,
    .llseek  = char_llseek,
    .release = char_release,
};


static int char_device_init(void) {
    int ret, i;
    dev_t curr_devt;
    struct device *char_device;

    /* Request for a major and CHAR_NBANK minors */
    ret = alloc_chrdev_region(&char_dev_num, 0, 
                DUMMY_CHAR_NBANK, 
                DUMMY_CHAR_DEVICE_NAME);
    if (ret < 0) {
        pr_err("Alloc chrdev failed\n");
        goto out;
    }
    /* create our device class, visible in /sys/class */
    char_class = class_create(THIS_MODULE, DUMMY_CHAR_CLASS);
    if (IS_ERR(char_class)) {
        pr_err("Class creation failed\n");
        ret = PTR_ERR(char_class);
        goto unreg_chrdev;
    }
    
    /* Each bank is represented as a character device (cdev) */
    for (i = 0; i < DUMMY_CHAR_NBANK; i++) {
        /* bind file_operations to the cdev */
        cdev_init(char_cdev + i, &fops);
        char_cdev[i].owner = THIS_MODULE;
        /* Device number to use to add cdev to the core */
        curr_devt = MKDEV(MAJOR(char_dev_num), MINOR(char_dev_num) + i);
        /* Make the device live for the users to access */
        ret = cdev_add(char_cdev + i, curr_devt, 1);
        if(ret < 0){
            pr_err("Cdev add failed\n");
            goto cdev_del;
        }
        /* create a node for each device */
        char_device = device_create(char_class, 
                                    NULL,       /* No parent */
                                    curr_devt,  /* associated dev_t */
                                    NULL,       /* No additional data */
                                    DUMMY_CHAR_DEVICE_NAME ": %d", i);
        if (IS_ERR(char_device)) {
            pr_err("Error creating dummy char device.\n");
            ret = PTR_ERR(char_device);
            goto class_del;
        }
    }
    pr_info("Module init was successful\n");

    return 0;

cdev_del:
class_del:
    for(; i >= 0; i--) {
        device_destroy(char_class, MKDEV(MAJOR(char_dev_num), MINOR(char_dev_num) + i));
        cdev_del(char_cdev + i);
    }
    class_destroy(char_class);
unreg_chrdev:
    unregister_chrdev_region(char_dev_num, DUMMY_CHAR_NBANK);
out:
    pr_info("Module insertion failed \n");
    return ret;
}


static void char_device_exit(void) {
    int i;
    for(i = 0; i < DUMMY_CHAR_NBANK; i++) {
        device_destroy(char_class, MKDEV(MAJOR(char_dev_num), MINOR(char_dev_num) + i));
        cdev_del(char_cdev + i);
    }
    class_destroy(char_class);
    unregister_chrdev_region(char_dev_num, DUMMY_CHAR_NBANK);

    pr_info("Dummy char device cleanup\n");
}


module_init(char_device_init);
module_exit(char_device_exit);


MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module creates dummy character device");
MODULE_INFO(modparams, "05-char_device");