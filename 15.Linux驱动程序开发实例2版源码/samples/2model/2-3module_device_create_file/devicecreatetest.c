
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/platform_device.h>

MODULE_LICENSE ("GPL");

static int fgj;

static ssize_t fgj_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	return sprintf(buf, "%d\n", fgj);
}

static ssize_t fgj_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	sscanf(buf, "%d", &fgj);
	return count;
}

static struct device_attribute fgj_attribute =
{
	.attr=
		{
		.name="fgj",
		.mode=0644,
		},
	.show=fgj_show,
	.store=fgj_store,
};

struct test_platform_data
{
int idx;
};
static struct test_platform_data test_info = {
	.idx = 25,
};

static void plate_test_release(struct device * dev)
{
    return ;
}

static struct platform_device plate_test_device = {
	.name   = "platetest",
	.id     = -1,
	.dev    = {
		.platform_data  = &test_info,
		.release =plate_test_release,
	},
};

static int  plate_test_probe(struct platform_device *pdev)
{
	printk (KERN_INFO "plate_test_probe enter...\n");
	device_create_file(&pdev->dev,&fgj_attribute);
	return 0;
}

static int  plate_test_remove(struct platform_device *pdev)
{
	printk (KERN_INFO "plate_test_probe remove...\n");
	return 0;
}

static struct platform_driver plate_test_driver = {
	.probe  = plate_test_probe,
	.remove = plate_test_remove,
	.driver = {
		.name   = "platetest",
		.owner  = THIS_MODULE,
	},
};

static int __init plateform_test_init (void)
{
	platform_device_register(&plate_test_device);
	platform_driver_register(&plate_test_driver);
	return 0;
}
static void __exit plateform_test_exit (void)
{
	platform_driver_unregister(&plate_test_driver);
	platform_device_unregister(&plate_test_device);
}
module_init (plateform_test_init);
module_exit (plateform_test_exit);
