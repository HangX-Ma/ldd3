#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/workqueue.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__

/* declare a waiting queue head */
DECLARE_WAIT_QUEUE_HEAD(work_queue);
static struct work_struct work_element;

static int condition = 0;

static void work_handler(struct work_struct *work) {
    pr_info("wait_queue module handler %s\n", __FUNCTION__);
    usleep_range(1000, 5000);
    condition = 1;
    pr_info("Woken event happened\n");
    wake_up_interruptible(&work_queue);
}


static int __init mywork_init(void) {
    pr_info("wait_queue module loaded \n");

    INIT_WORK(&work_element, work_handler);
    schedule_work(&work_element);

    pr_info("Going to sleep %s\n", __FUNCTION__);
    wait_event_interruptible(work_queue, (condition != 0));
    pr_info("Woken up from sleep \n");
    
    return 0;
}


static void __exit mywork_exit(void) {
    /*  ensure that any scheduled work has run to completion */
    flush_scheduled_work();
    pr_info("wait_queue module cleanup \n");
}

module_init(mywork_init);
module_exit(mywork_exit);

MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module has waiting queue and timer elements");
MODULE_INFO(modparams, "02-wait_queue");