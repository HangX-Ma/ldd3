#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

/* The delay for the delayed workqueue timer file. */
static long delay = 1;
module_param(delay, long, 0);

/**
 * This module is a silly one: it only embeds short code fragments
 * that show how enqueued tasks `feel' the environment
 */

#define LIMIT   (PAGE_SIZE-128)	/* don't print any more after this size */

/**
 * Print information about the current environment. This is called from
 * within the task queues. If the limit is reched, awake the reading
 * process.
 */
static DECLARE_WAIT_QUEUE_HEAD(jiq_wait);

/**
 * Keep track of info we need between task queue runs.
 */
static struct clientdata {
    struct work_struct    jiq_work;
    struct delayed_work   jiq_delayed_work;
    struct timer_list     jiq_timer;
    struct tasklet_struct jiq_tasklet;
    struct seq_file       *m;
    int                   len;
    unsigned long         jiffies;
    long                  delay;
} jiq_data;


static int jiq_print(struct clientdata *data)
{
    int len = data->len;
    struct seq_file *m = data->m;
    unsigned long j = jiffies;

    if (len > LIMIT) {
        wake_up_interruptible(&jiq_wait);
        return 0;
    }

    if (len == 0) {
        seq_puts(m, "    time  delta preempt   pid cpu command\n");
        len = m->count;
    } else {
        len = 0;
    }

    /* intr_count is only exported since 1.3.5,
        but 1.99.4 is needed anyways */
    seq_printf(m, "%9li  %4li     %3i %5i %3i %s\n",
            j, j - data->jiffies,
            preempt_count(), current->pid, smp_processor_id(),
            current->comm);
    len += m->count;

    data->len += len;
    data->jiffies = j;
    return 1;
}

/* ---------------------------- 1. jit workqueue ---------------------------- */
static void jiq_print_wq(struct work_struct *work) {
    struct clientdata *data = container_of(work, struct clientdata, jiq_work);
    if (!jiq_print(data)) {
        return;
    }

    schedule_work(&jiq_data.jiq_work);
}

int jiq_read_wq_show (struct seq_file *m, void *v) {
    /* define a wait_queue_entry */
    DEFINE_WAIT(wait);

    jiq_data.len = 0;                /* nothing printed, yet */
    jiq_data.m = m;                  /* print in this place */
    jiq_data.jiffies = jiffies;      /* initial time */
    jiq_data.delay = 0;

    prepare_to_wait(&jiq_wait, &wait, TASK_INTERRUPTIBLE);
    /** 
     * When a process reads /proc/jiqwq, the module initiates 
     * a series of trips through the shared workqueue with no delay
     */
    schedule_work(&jiq_data.jiq_work);
    /* Once schedule returns, it is cleanup time. */
    schedule();
    finish_wait(&jiq_wait, &wait);
    
    return 0;
}

int jiq_read_wq_open (struct inode *inode, struct file *filp) {
    return single_open(filp, jiq_read_wq_show, NULL);
}


static struct proc_ops jiq_read_wq_proc_ops = {
    .proc_open    = jiq_read_wq_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release
};






/* -------------------------- 2. jit delayed workqueue -------------------------- */
static void jiq_print_wq_delayed(struct work_struct *work) {
    struct clientdata *data = container_of(work, struct clientdata, jiq_delayed_work.work);
    if (!jiq_print(data)) {
        return;
    }

    schedule_delayed_work(&jiq_data.jiq_delayed_work, data->delay);
}

static int jiq_read_wq_delayed_show(struct seq_file *m, void *v) {
    DEFINE_WAIT(wait);

    jiq_data.len = 0;                /* nothing printed, yet */
    jiq_data.m = m;                  /* print in this place */
    jiq_data.jiffies = jiffies;      /* initial time */
    jiq_data.delay = delay;

    prepare_to_wait(&jiq_wait, &wait, TASK_INTERRUPTIBLE);
    schedule_delayed_work(&jiq_data.jiq_delayed_work, delay);
    schedule();
    finish_wait(&jiq_wait, &wait);

    return 0;
}

static int jiq_read_wq_delayed_open(struct inode *inode, struct file *file) {
    return single_open(file, jiq_read_wq_delayed_show, NULL);
}

static struct proc_ops jiq_read_wq_delayed_proc_ops = {
    .proc_open    = jiq_read_wq_delayed_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release
};


static int __init jiq_init(void) {
    INIT_WORK(&jiq_data.jiq_work, jiq_print_wq);
    INIT_DELAYED_WORK(&jiq_data.jiq_delayed_work, jiq_print_wq_delayed);

    proc_create("jiqwq", 0, NULL, &jiq_read_wq_proc_ops);
    proc_create("jiqwqdelay", 0, NULL, &jiq_read_wq_delayed_proc_ops);


    return 0;
}

static void __exit jiq_exit(void) {
    remove_proc_entry("jiqwq", NULL);
    remove_proc_entry("jiqwqdelay", NULL);
}




module_init(jiq_init);
module_exit(jiq_exit);


MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module about time, delay and deffer work, focusing on shared queue.");
MODULE_INFO(modparams, "11-jiq");