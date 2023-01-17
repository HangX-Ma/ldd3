/**
 * @file jit.c
 * @brief Just-in-time module
 */

#include "jit.h"

int delay = HZ; /* the default delay, expressed in jiffies */

module_param(delay, int, 0);
/* use these as data pointers, to implement four files in one function */
enum fn_ops {
    JIT_BUSY,
    JIT_SCHED,
    JIT_QUEUE,
    JIT_SCHEDTO
};


/** ******************************* currenttime *******************************
 * Show the current jiffies in the following three excerpts:
 *  - The current jiffies and jiffies_64 values as hex numbers.
 *  - The current time as returned by ktime_get_real_ts64 (replace do_gettimeofday)
 *  - The timespec returned by ktime_get_coarse_real_ts64 (replace current_kernel_time)
 */
static int jit_ctime_show (struct seq_file *m, void *v) {
    unsigned long j1;
    u64 j2;

    struct timespec64 tv1;
    struct timespec64 tv2;

    /* get current timestamp in different types */
    j1 = jiffies;
    j2 = get_jiffies_64();

    ktime_get_real_ts64(&tv1);
    ktime_get_coarse_real_ts64(&tv2);

    seq_printf(m, "0x%08lx 0x%016Lx %10lli.%09li\n"
                    "%40lli.%09li\n",
                    j1, j2,
                    tv1.tv_sec, tv1.tv_nsec,
                    tv2.tv_sec, tv2.tv_nsec);
    return 0;
}

int jit_ctime_proc_open (struct inode *inode, struct file *filp) {
    /** 
     * using single open can simplify the kernel interface,
     * so that we can spare the efforts to realize the 
     * entire seq_operations.
     */
    return single_open(filp, &jit_ctime_show, NULL);
}

/* 'current time' /proc operations */
static struct proc_ops jit_ctime_proc_ops = {
    .proc_open    = jit_ctime_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

// --------------------------------------------------------------------------
//                                 LONG DELAY
// --------------------------------------------------------------------------


/** ******************************* N operations set *******************************
 * This function prints one line of data, after sleeping one second.
 * It can sleep in different ways, according to the data pointer.
 */
static int jit_fn_show (struct seq_file *m, void *v) {
    /* Get data from caller */
    unsigned long j0, j1;
    wait_queue_head_t wait;

    long data = (long)m->private;

    j0 = jiffies;
    j1 = jiffies + delay;

    switch (data) {
        case JIT_BUSY:
            while (time_before(jiffies, j1)) {
                /** This approach should definitely be avoid, who waste 
                    the resource of CPU unless using preemptive operation. */
                cpu_relax(); // Jiffies will not be updated if interrupts disabled
            }
            break;
        case JIT_SCHED:
            while (time_before(jiffies, j1)) {
                /** 
                 * - The current process does nothing but release the CPU, but it 
                 * remains in the run queue. What's more, each 'read' sometimes
                 * ends up waiting a few clock ticks more than requested, which
                 * gets worse when system getting busy. 
                 * 
                 * - Only one runnable process causes redundant entry of this loop.
                 */
                schedule();
            }
            break;
        case JIT_QUEUE:
            /**
             * These functions sleep on the given wait queue, but they return after
             * the timeout (expressed in jiffies) expires.
             */
            init_waitqueue_head(&wait);
            wait_event_interruptible_timeout(wait, 0, delay);
            break;
        case JIT_SCHEDTO:
            /**
             * To accommodate for this very situation, where you want to delay execution
             * waiting for no specific event, schedule_timeout function is used.
             */ 
            set_current_state(TASK_INTERRUPTIBLE); // the scheduler won’t run the current 
                                                   // process again until the timeout places 
                                                   // it back in TASK_RUNNING state. 
            schedule_timeout (delay);
            break;
    }
    j1 = jiffies; /* actual value after we delayed */

    seq_printf(m, "%9li %9li\n", j0, j1);
    return 0;
}

int jit_fn_proc_open (struct inode *inode, struct file *filp) {
    /* PDE_DATA is a wrapper of container_of */
    return single_open(filp, &jit_fn_show, PDE_DATA(inode));
}

static struct proc_ops jit_fn_proc_ops = {
    .proc_open    = jit_fn_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

// --------------------------------------------------------------------------
//                                 SHORT DELAY
// --------------------------------------------------------------------------

/*
    void ndelay(unsigned long nsecs);
    void udelay(unsigned long usecs);
    void mdelay(unsigned long msecs);
    void msleep(unsigned int millisecs);
    unsigned long msleep_interruptible(unsigned int millisecs);
    void ssleep(unsigned int seconds)
*/

// --------------------------------------------------------------------------
//                                 KERNEL TIMES
// --------------------------------------------------------------------------

/**
 * Kernel timers are run as the result of a “software interrupt.” When running 
 * in this sort of atomic context, your code is subject to a number of constraints.
 * 
 *  1. No access to user space is allowed. Because there is no process context, 
 *     there is no path to the user space associated with any particular process.
 * 
 *  2. The current pointer is not meaningful in atomic mode and cannot be used since 
 *     the relevant code has no connection with the process that has been interrupted.
 * 
 *  3. No sleeping or scheduling may be performed. Atomic code may not call schedule 
 *     or a form of wait_event, nor may it call any other function that could sleep. 
 *     For example, calling kmalloc(..., GFP_KERNEL) is against the rules. Semaphores 
 *     also must not be used since they can sleep.
 */

int tdelay = 10;
module_param(tdelay, int, 0);

#define JIT_ASYNC_LOOPS 5

/* This data structure used as "data" for the timer and tasklet functions */
struct jit_data {
    struct timer_list     timer;
    struct tasklet_struct tlet;
    struct seq_file       *m;
    int                   hi; /* tasklet or tasklet_hi */
    wait_queue_head_t     wait;
    unsigned long         prevjiffies;
    int                   loops;
};


void jit_timer_fn(struct timer_list *t) {
    struct jit_data *data = from_timer(data, t, timer); // act as container_of macro
    unsigned long j = jiffies;
    seq_printf(data->m, "%9li  %3li     %i    %6i   %i   %s\n",
                    j, j - data->prevjiffies, in_interrupt() ? 1 : 0,
                    current->pid, smp_processor_id(), current->comm);

    if (--data->loops) {
        data->timer.expires += tdelay; // update expires time value
        data->prevjiffies = j;
        add_timer(&data->timer);
    } else {
        wake_up_interruptible(&data->wait);
    }
}



/* the /proc function: allocate everything to allow concurrency */
int jit_timer_show(struct seq_file *m, void *v) {
    struct jit_data *data;
    unsigned long j = jiffies;

    data = kmalloc(sizeof(*data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }

    init_waitqueue_head(&data->wait);

    /* write the first lines in the buffer */
    seq_puts(m, "   time   delta  inirq    pid   cpu command\n");
    seq_printf(m, "%9li  %3li     %i    %6i   %i   %s\n",
            j, 0L, in_interrupt() ? 1 : 0,
            current->pid, smp_processor_id(), current->comm);
    
    /* fill the data for our timer function */
    data->prevjiffies = j;
    data->m = m;
    data->loops = JIT_ASYNC_LOOPS;

    /* register the timer */
    timer_setup(&data->timer, jit_timer_fn, 0);
    data->timer.expires = j + tdelay; /* parameter */
    add_timer(&data->timer);

    /* wait for the buffer to fill */
    wait_event_interruptible(data->wait, !data->loops);
    /**
     * The call to signal_pending tells us whether we were awakened 
     * by a signal; if so, we need to return to the user and let them 
     * try again later.
     */
    if (signal_pending(current)) {
        return -ERESTARTSYS;
    }
    kfree(data);
    return 0;
}

static int jit_timer_open(struct inode *inode, struct file *file) {
    return single_open(file, jit_timer_show, NULL);
}

static struct proc_ops jit_timer_fops = {
    .proc_open    = jit_timer_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};



/**
 * Tasklet are always run at interrupt time, they always run on the same 
 * CPU that schedules them, and they receive an unsigned long argument.
 * 
 */

void jit_tasklet_fn(unsigned long arg) {
    struct jit_data *data = (struct jit_data *)arg;
    unsigned long j = jiffies;
    seq_printf(data->m, "%9li  %3li     %i    %6i   %i   %s\n",
                    j, j - data->prevjiffies, in_interrupt() ? 1 : 0,
                    current->pid, smp_processor_id(), current->comm);

    if (--data->loops) {
        data->prevjiffies = j;
        if (data->hi)
            tasklet_hi_schedule(&data->tlet);
        else
            tasklet_schedule(&data->tlet);
    } else {
        wake_up_interruptible(&data->wait);
    }
}

/* the /proc function: allocate everything to allow concurrency */
int jit_tasklet_show(struct seq_file *m, void *v) {
    struct jit_data *data;
    unsigned long j = jiffies;
    /* The jitasklethi implementation uses a high-priority tasklet */
    long hi = (long)m->private;

    data = kmalloc(sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    init_waitqueue_head(&data->wait);

    /* write the first lines in the buffer */
    seq_puts(m, "   time   delta  inirq    pid   cpu command\n");
    seq_printf(m, "%9li  %3li     %i    %6i   %i   %s\n",
            j, 0L, in_interrupt() ? 1 : 0,
            current->pid, smp_processor_id(), current->comm);

    /* fill the data for our tasklet function */
    data->prevjiffies = j;
    data->m = m;
    data->loops = JIT_ASYNC_LOOPS;

    /* register the tasklet */
    tasklet_init(&data->tlet, jit_tasklet_fn, (unsigned long)data);
    data->hi = hi;
    if (hi)
        tasklet_hi_schedule(&data->tlet);
    else
        tasklet_schedule(&data->tlet);

    /* wait for the buffer to fill */
    wait_event_interruptible(data->wait, !data->loops);

    if (signal_pending(current))
        return -ERESTARTSYS;
    kfree(data);
    return 0;
}

static int jit_tasklet_open(struct inode *inode, struct file *file) {
    return single_open(file, jit_tasklet_show, PDE_DATA(inode));
}

static struct proc_ops jit_tasklet_fops = {
    .proc_open    = jit_tasklet_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};


/* *************************************************************** */

static int __init jit_init(void) {

    proc_create_data("currentime", 0, NULL, &jit_ctime_proc_ops, NULL);
    proc_create_data("jitbusy", 0, NULL, &jit_fn_proc_ops, (void *)JIT_BUSY);
    proc_create_data("jitsched", 0, NULL, &jit_fn_proc_ops, (void *)JIT_SCHED);
    proc_create_data("jitqueue", 0, NULL, &jit_fn_proc_ops, (void *)JIT_QUEUE);
    proc_create_data("jitschedto", 0, NULL, &jit_fn_proc_ops, (void *)JIT_SCHEDTO);

    proc_create_data("jitimer", 0, NULL, &jit_timer_fops, NULL);
    proc_create_data("jitasklet", 0, NULL, &jit_tasklet_fops, NULL);
    proc_create_data("jitasklethi", 0, NULL, &jit_tasklet_fops, (void *)1);

    return 0;
}

static void __exit jit_exit(void) {
    remove_proc_entry("currentime", NULL);
    remove_proc_entry("jitbusy", NULL);
    remove_proc_entry("jitsched", NULL);
    remove_proc_entry("jitqueue", NULL);
    remove_proc_entry("jitschedto", NULL);

    remove_proc_entry("jitimer", NULL);
    remove_proc_entry("jitasklet", NULL);
    remove_proc_entry("jitasklethi", NULL);
}


module_init(jit_init);
module_exit(jit_exit);


MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module about time, delay and deffer work");
MODULE_INFO(modparams, "11-jit");
