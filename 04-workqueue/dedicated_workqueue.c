#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h> /* kernel memory management */

static struct workqueue_struct *dedicated_wq;

enum work_status {
    IDLE,
    RUNNING,
};

struct work_data_t {
    struct work_struct work;
    int state;
};


static void dedicated_work_handler(struct work_struct *pwork) {
    struct work_data_t * work_data = container_of(pwork, struct work_data_t, work);
    work_data->state = RUNNING;
    pr_info("Dedicated work queue handler: %s\n", work_data->state ? "RUNNING" : "IDLE");

    /* release memory */
    kfree(work_data);
}


static int __init dedicated_wq_init(void) {
    struct work_data_t *work_data;

    pr_info("Dedicated work queue module init: %d\n", __LINE__);

    /* create single thread work queue */
#if !CMWQ
    dedicated_wq = create_singlethread_workqueue("single thread work queue");
#else
    /* CMWQ (Concurrency Managed Workqueue) */
    dedicated_wq = alloc_workqueue("cmwp-example",
                         /* we want it to be unbound and high priority */
                         WQ_UNBOUND | WQ_HIGHPRI,
                         0);
#endif
    work_data = kmalloc(sizeof(struct work_data_t), GFP_KERNEL);
    work_data->state = IDLE;

    INIT_WORK(&work_data->work, dedicated_work_handler);
    
    /* schedule works on the created workqueue */
    queue_work(dedicated_wq, &work_data->work);

    return 0;
}

static void __exit dedicated_wq_exit(void) {
    flush_workqueue(dedicated_wq);
    destroy_workqueue(dedicated_wq);

    pr_info("Dedicated work queue cleanup\n");
}

module_init(dedicated_wq_init);
module_exit(dedicated_wq_exit);

MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module has dedicated work queue");
MODULE_INFO(modparams, "04-workqueue: dedicated");
