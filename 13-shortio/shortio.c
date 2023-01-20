#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define SHORT_NR_PORTS  1
#define SHORT_MOD_NAME  "shortio"

static unsigned long port_base = 0x378;


static int 
shortio_open(struct inode *inode, struct file *filp) {
    pr_debug("%s is invoked.\n", __func__);
    return 0;
}

static int 
shortio_release(struct inode *inode, struct file *filp) {
    pr_debug("%s is invoked.\n", __func__);
    return 0;
}

static ssize_t
shortio_read(struct file *filp, char __user *buf, size_t count, loff_t *fpos) {
    char v;

    if (*fpos > 0) {
        return 0;
    }

    v = inb(port_base);
    rmb();

    (*fpos)++;

    if (copy_to_user(buf, &v, 1)) {
        return -EFAULT;
    }

    return 1;
}

static ssize_t 
shortio_write(struct file *filp, const char __user *buf, size_t count, loff_t *fpos) {
    unsigned char *kbuf, *ptr;
    size_t cnt = count;

    kbuf = kmalloc(cnt, GFP_KERNEL);

    if (!kbuf) {
        return -ENOMEM;
    }
    if (copy_from_user(kbuf, buf, cnt))
        return -EFAULT;

    ptr = kbuf;

    while (cnt--) {
        pr_debug("port=%lu(%#lx), val=%d(%#x)\n", port_base, port_base, *ptr, *ptr);
        outb(*(ptr++), port_base);
        wmb();
    }
    kfree(kbuf);

    return count;
}


static const struct file_operations shortio_fops = {
    .owner   = THIS_MODULE,
    .open    = shortio_open,
    .read    = shortio_read,
    .write   = shortio_write,
    .release = shortio_release
};


static dev_t shortio_dev_num;
static struct cdev shortio_cdev;

static int __init shortio_init(void) {
    int ret;

    /* Initialize I/O. All port allocations show up in /proc/ioports.*/
    if (!request_region(port_base, SHORT_NR_PORTS, SHORT_MOD_NAME)) {
        pr_err("%s: cannot get I/O port address %#lx", __func__, port_base);
        ret = -ENODEV;
        goto out;
    }

    ret = alloc_chrdev_region(&shortio_dev_num, 0, SHORT_NR_PORTS, SHORT_MOD_NAME);
    if (ret < 0) {
        pr_err("Allocate chrdev failed.\n");
        goto out;
    }
    cdev_init(&shortio_cdev, &shortio_fops);
    cdev_add(&shortio_cdev, MKDEV(MAJOR(shortio_dev_num), MINOR(shortio_dev_num)), 1);
    if (ret) {
        pr_err("Error %d adding shortio\n", ret);
        goto unreg_cdev;
    }

    pr_info("shortio module is initialized successfully\n");

    return 0;
unreg_cdev:
    unregister_chrdev_region(shortio_dev_num, SHORT_NR_PORTS);
out:
    return ret;
}

static void __exit shortio_exit(void) {
    cdev_del(&shortio_cdev);
    unregister_chrdev_region(shortio_dev_num, SHORT_NR_PORTS);
    release_region(port_base, SHORT_NR_PORTS);
}

module_init(shortio_init);
module_exit(shortio_exit);


MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module communicates with hardware I/O ports.");
MODULE_INFO(modparams, "13-shortio");