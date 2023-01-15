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
#include <linux/poll.h>

#ifndef SCULL_P_NR_DEVS
#define SCULL_P_NR_DEVS     4
#endif

#ifndef SCULL_P_MODULE_NAME
#define SCULL_P_MODULE_NAME  "scull_pipe"
#endif

// The pipe device is a simple circular buffer.
#ifndef SCULL_P_BUFFER
#define SCULL_P_BUFFER      4000
#endif

struct scull_pipe {
    wait_queue_head_t inq, outq;        /* read and write queues */
    char *buffer, *end;                 /* begin of buf, end of buf */
    int buffersize;                     /* used in pointer arithmetic */
    char *rp, *wp;                      /* where to read, where to write */
    int nreaders, nwriters;             /* number of openings for r/w */
    struct fasync_struct *async_queue;  /* asynchronous readers */
    struct mutex mlock;                 /* mutual exclusion mutex */
    struct cdev cdev;                   /* Char device structure */
};

#endif  //!__SCULL__H__