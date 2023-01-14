#ifndef __SCULL__H__
#define __SCULL__H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/moduleparam.h>

/* format the print function */
#undef pr_fmt
#define pr_fmt(fmt) "%s " fmt, __func__

/* scull module properties */
#define SCULL_NR_DEVS       3
#define SCULL_MODULE_NAME   "scull"

#ifndef SCULL_QUANTUM
#define SCULL_QUANTUM 400
#endif

#ifndef SCULL_QSET
#define SCULL_QSET    10
#endif

struct scull_qset {
    void **data;
    struct scull_qset *next;
};

struct scull_dev {
    struct scull_qset *data;    /* Pointer to first quantum set */
    int quantum;                /* the current quantum size */
    int qset;                   /* the current qset size */
    unsigned long size;         /* amount of data stored here */
    unsigned int access_key;
    struct cdev cdev;           /* Char device structure */
    struct mutex mlock;         /* mutual exclusion semaphore */
};

/* file_operation template */
ssize_t scull_read (struct file *filp, char __user *buf, size_t count, loff_t *fpos);
ssize_t scull_write (struct file *filp, const char __user *buf, size_t count, loff_t *fpos);
int scull_open (struct inode *inode, struct file *filp);
int scull_release (struct inode *inode, struct file *filp);
int scull_trim(struct scull_dev *dev);

extern int scull_nr_devs;
extern int scull_quantum;
extern int scull_qset;

#ifdef SCULL_DEBUG /* use proc only if debugging */
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#endif

#endif  //!__SCULL__H__