#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__

/* Create a struct for passing additional arguments to timer callback func */
struct fake_data_t {
    struct timer_list low_res_timer;
    bool condition;
};

struct fake_data_t fake_data = {
    .condition = 0
};

static struct hrtimer hr_timer;

static void low_res_timer_callback( struct timer_list * timer) {
    int retval;
    /* acquire data from fake_data_t structure , 
        from_timer() is actually the same as the container_of() marco */
    struct fake_data_t *inner_fd = from_timer(&fake_data, timer, low_res_timer);
    // struct fake_data_t *inner_fd = container_of(timer, struct fake_data_t, low_res_timer);

    pr_info("Low-res timer callback \n");

    inner_fd->condition = ! inner_fd->condition;
    pr_info("fake_data.condition = %d\n", (int)inner_fd->condition);
    if (inner_fd->condition == 0) {
        pr_info("Low-res resolution timer stops now!\n");
        return;
    }

    pr_info( "Setup low-res timer to fire in another 500ms (%ld)\n", jiffies);
    retval = mod_timer(timer, jiffies + msecs_to_jiffies(500));
    if (retval) {
        pr_info("Low-res timer firing failed\n");
    }
}

enum hrtimer_restart hrtimer_callback(struct hrtimer *timer) {
    pr_info("called (%ld). HRTIMER_NORESTART\n", jiffies);
    return HRTIMER_NORESTART;
}


static int __init timer_init(void) {
    ktime_t ktime;
    s64 ms, ns;

    ktime = ktime_set(0, 10000000);
    ms = ktime_to_ms(ktime);
    ns = ktime_to_ns(ktime);

    pr_info("Timer module is detected! \n");

    pr_info("High resolution timer ktime (%lld ms) or (%lld ns), jiffies(%ld)\n", ms, ns, jiffies);

    /* 1. Initialize the timer */
    hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hr_timer.function = &hrtimer_callback;
    timer_setup(&fake_data.low_res_timer, low_res_timer_callback, 0);

    /* 2. Set timer expiration delay before timer start */
    fake_data.low_res_timer.expires = jiffies + (1 * (HZ)/250); /* 1.0s */
    
    /* 3. One time timer registration */
    hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);
    add_timer(&fake_data.low_res_timer);
    
    return 0;
}

static void __exit timer_exit(void) {
    int retval;
    /* Unregister the timer */
    retval = del_timer_sync(&fake_data.low_res_timer);
    if (retval) {
        pr_info("The timer is still in use...\n");
    }

    retval = hrtimer_cancel(&hr_timer);
    if (retval) {
        pr_info("The timer is still in use...\n");
    }

    pr_info("timer module cleanup \n");
}


module_init(timer_init);
module_exit(timer_exit);

MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module has low-resolution and high-resolution timer");
MODULE_INFO(modparams, "03-timer");
