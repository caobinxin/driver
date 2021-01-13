#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>


extern int function_of_dep(void);

static int __init demo_module_init(void)
{
	printk("simple module init\n");
	function_of_dep();
	return 0;
}

static void __exit demo_module_exit(void)
{
	printk("simple module exit\n");
}
module_init(demo_module_init);
module_exit(demo_module_exit);

MODULE_AUTHOR("fgjnew <fgjnew@163.com>");
MODULE_DESCRIPTION("simple module");
MODULE_LICENSE("GPL");

