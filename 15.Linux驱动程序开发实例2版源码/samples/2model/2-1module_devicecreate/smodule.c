/* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*
*/
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

MODULE_LICENSE ("GPL");
int simple_major = 250;
int simple_minor = 0;
int number_of_devices = 1;
struct cdev cdev;
dev_t dev = 0;
struct file_operations simple_fops = {
	.owner = THIS_MODULE,
};
struct class *simple_class;
static void simple_register (void)
{
	int error, devno = MKDEV (simple_major, simple_minor);
	cdev_init (&cdev, &simple_fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &simple_fops;
	error = cdev_add (&cdev, devno , 1);
	if (error)
		printk (KERN_NOTICE "cdev_add error %d", error);

	simple_class =class_create(THIS_MODULE, "simple_class");
	if(IS_ERR(simple_class)) {
		printk("Err: failed in creating class.\n");
		return ;
	}
	
	device_create(simple_class,NULL, devno, NULL,"simple");
}
static int __init simple_init (void)
{
	int result;
	dev = MKDEV (simple_major, simple_minor);
	result = register_chrdev_region (dev, number_of_devices, "simple");
	if (result<0) {
		printk (KERN_WARNING "hello: can't get major number %d\n", simple_major);
		return result;
	}
	simple_register();
	printk (KERN_INFO "char device registered\n");
	return 0;
}
static void __exit simple_exit (void)
{
	dev_t devno = MKDEV (simple_major, simple_minor);
	cdev_del (&cdev);
	unregister_chrdev_region (devno, number_of_devices);
	device_destroy(simple_class, devno);
	class_destroy(simple_class);
}
module_init (simple_init);
module_exit (simple_exit);
