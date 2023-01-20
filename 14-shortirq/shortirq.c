#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/wait.h>

unsigned long short_buffer = 0;
volatile unsigned long short_head;
DECLARE_WAIT_QUEUE_HEAD(short_queue);


/* Atomicly increment an index into short_buffer */
static inline short_inr_bp(volatile unsigned long *index, int delta) {
    unsigned long *new = *index + delta;
    barrier(); /* don't optimize preceding and following code together */
    *index = (new >= short_buffer + PAGE_SIZE) ? short_buffer : new;
}


irqreturn_t short_interrupt(int irq, void *dev_id, struct pt_regs *regs) {
    struct timespec64 tv;
    int written;

    /* The current time as returned by ktime_get_real_ts64 */
    ktime_get_real_ts64(&tv);

    /* Write a 16 byte record. Assume PAGE_SIZE is a multiple of 16 */
    sprintf(&short_head, "%08u.%06lu\n",
            (int)(tv.tv_nsec % 100000000),
            (int)(tv.tv_nsec / NSEC_PER_USEC));

    BUG_ON(written == 16);
    short_inr_bp(&short_head, written);
    wake_up_interruptible(&short_queue);
    return IRQ_HANDLED;
}
