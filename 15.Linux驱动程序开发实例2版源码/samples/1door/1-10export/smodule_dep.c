#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>

int function_of_dep(void)
{	
	printk("function_of_dep\n");
	return 0;
}
EXPORT_SYMBOL(function_of_dep);

static int __init demo_module_init(void)
{
	printk("simple module dep init\n");
	return 0;
}

static void __exit demo_module_exit(void)
{
	printk("simple module dep exit\n");
}
module_init(demo_module_init);
module_exit(demo_module_exit);

MODULE_AUTHOR("fgjnew <fgjnew@163.com>");
MODULE_DESCRIPTION("simple module dep");
MODULE_LICENSE("GPL");

