#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

static int   mint    = 9;
static char *mchar   = "char";
static int   marr[2] = {0, 1};

MODULE_PARM_DESC(mint, "Integer variable");
MODULE_PARM_DESC(mchar, "Character pointer variable");
MODULE_PARM_DESC(marr, "Integer variable array");

/* Permission is consist of prefix 'S_I', with intermediate 'R', 'W', 'X'，
   suffix 'USR', 'GPR', 'UGO', 'OTH' */
module_param(mint, int, S_IRUGO);
module_param(mchar, charp, S_IRUGO);
module_param_array(marr, int, NULL, S_IRUSR | S_IWUSR);

/* __init and __exit are Linux directives (macros) that wrap GNU C 
compiler attributes used for symbol placement. */
static int __init modparams_init(void) {
    pr_info("module_params is loaded!\n");
    pr_info("The *mint* parameter: %d\n", mint);
    pr_info("The *mchar* parameter: %s\n", mchar);
    pr_info("The *marr* parameter: %d, %d\n", marr[0], marr[1]);

    return 0;
}

static void __exit modparams_exit(void) {
    pr_info("module_params is unloaded.\n");
}

/* These are two macro for module initialization and removal */
module_init(modparams_init);
module_exit(modparams_exit);

/* adds generic information of the tag = "info" form. */
MODULE_AUTHOR("HangX-Ma");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("A module needs several parameters");
MODULE_INFO(modparams, "01-module_params");
