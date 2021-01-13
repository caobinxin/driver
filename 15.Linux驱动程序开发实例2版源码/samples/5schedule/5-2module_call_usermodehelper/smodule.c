#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>


static int demo_module_init(void)
{
	int ret;
	char *argv[5], *envp[3];

	argv[0] = "/bin/mkdir";
	argv[1] = "/home/a/a";
	argv[2] = NULL;
	argv[3] = NULL;
	argv[4] = NULL;

	envp[0] = "HOME=/";
	envp[1] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
	envp[2] = NULL;

	ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
	if (ret < 0) {
		printk(KERN_ERR
		       "Error %d running user helper "
		       "\"%s %s %s %s\"\n",
		       ret, argv[0], argv[1], argv[2], argv[3]);
	}
	return 0;
}

static void demo_module_exit(void)
{
	printk("demo_module_exit\n");
}
module_init(demo_module_init);
module_exit(demo_module_exit);

MODULE_DESCRIPTION("simple module");
MODULE_LICENSE("GPL");

