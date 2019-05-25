#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include "virtualio.h"

static int virtualio_major = 0 ;
static int virtualio_minor = 0 ;

static struct class* virtualio_class = NULL ;
static struct virtualio_android_dev *virtualio_dev = NULL ;

static int virtualio_open(struct inode *inode, struct file *filp) ;
static int virtualio_release(struct inode *inode, struct file *filp) ;
static ssize_t virtualio_read(struct file *filp, char __user *buf, size_t num, loff_t *loff) ;
static ssize_t virtualio_write(struct file *filp, const char __user *buf, size_t num, loff_t *loff) ;

static struct file_operations virtualio_fops = {
	.owner = THIS_MODULE, 
	.open = virtualio_open, 
	.release= virtualio_release, 
	.read = virtualio_read, 
	.write = virtualio_write,
} ;

static int virtualio_open(struct inode *inode, struct file *filp)
{
	struct virtualio_android_dev *dev ;

	dev = container_of(inode->i_cdev, struct virtualio_android_dev, dev) ;
	filp->private_data = dev ;

    printk(KERN_INFO"virtualio_open...\n") ;

	return 0 ;
}


static int virtualio_release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO"virtualio_release...\n") ;
	return 0 ;
}


static ssize_t virtualio_read(struct file *filp, char __user *buf, size_t num, loff_t *loff) 
{
	ssize_t err = 0 ;
	struct virtualio_android_dev *dev = filp->private_data ;
    int num_i = (int)num ;

    printk(KERN_INFO"virtualio_read...\n") ;
    printk(KERN_INFO"num = %d\n", (int)num) ;
    

#if 1

	// if(down_interruptible(&(dev->sem))){
	// 	return -ERESTARTSYS ;
	// }

	if(num_i < 1 || num_i > 4){
		goto out ;
	}

    printk(KERN_INFO"reg= %d %d %d %d ...\n", dev->reg1, dev->reg2, dev->reg3, dev->reg4) ;

    #if 1
	switch (num_i){
		case 1:
			err = copy_to_user((char *)buf, (void *)&(dev->reg1), 1) ;
			break ;
		case 2:
			err = copy_to_user(buf + 1, &(dev->reg2), 1) ;
			break ;
		case 3:
			err = copy_to_user(buf + 2, &(dev->reg3), 1) ;
			break ;
		case 4:
			err = copy_to_user(buf + 3, &(dev->reg4), 1) ;
			break ;
		default :
			err = -EFAULT ;
			goto out ;
			break ;
	}
    #endif

	err = num_i ;

out:
	// up(&(dev->sem)) ;
	return err ;
#endif
}

static ssize_t virtualio_write(struct file *filp, const char __user *buf, size_t num, loff_t *loff)
{
	struct virtualio_android_dev *dev = filp->private_data ;
	ssize_t err = 0 ;
    int num_i = (int)num ;

    printk(KERN_INFO"virtualio_write...\n") ;
    printk(KERN_INFO"num = %d\n", (int)num) ;

#if 1

	// if(down_interruptible(&(dev->sem))){
	// 	return -ERESTARTSYS ;
	// }

	if(num_i < 1 || num_i > 4){
		goto out ;
	}

	switch(num_i){
		case 1:
			err = copy_from_user(&(dev->reg1), buf, 1) ;
			break ;
		case 2:
			err = copy_from_user(&(dev->reg2), buf, 1) ;
			break ;
		case 3:
			err = copy_from_user(&(dev->reg3), buf, 1) ;
			break ;
		case 4:
			err = copy_from_user(&(dev->reg4), buf, 1) ;
			break ;
		default:
			err = -EFAULT ;
			goto out ;
			break ;
	}
	err = num ;

out:
	// up(&(dev->sem)) ;
	return err ;
#endif

}

static const struct file_operations virtualio_proc_fops = {                                                                                                  
    .owner = THIS_MODULE, 
	.open = virtualio_open, 
	.release= virtualio_release, 
	.read = virtualio_read, 
	.write = virtualio_write,
};


static void virtualio_create_proc(void)
{
	struct proc_dir_entry *entry ;
#if 1
    entry = proc_create("VIRTUALIO_DEVICE_PROC_NAME", 0, NULL, &virtualio_proc_fops);

#else
    //很旧的函数接口 现在废弃了
	entry = create_proc_entry(VIRTUALIO_DEVICE_PROC_NAME, 0, NULL) ;
	if(entry){
		entry->owner = THIS_MODULE ;
		entry->read_proc = virtualio_proc_read ;
		entry->write_proc = virtualio_proc_write ;
	}
#endif
}

static void virtualio_remove_proc(void)
{
	remove_proc_entry(VIRTUALIO_DEVICE_PROC_NAME, NULL) ;
}

static int __virtualio_setup_dev(struct virtualio_android_dev *dev)
{
	int err ;
	DEFINE_SEMAPHORE(semaphore_virtualio) ;
	dev_t devno = MKDEV(virtualio_major, virtualio_minor) ;

	memset(dev, 0, sizeof(struct virtualio_android_dev)) ;

	cdev_init(&(dev->dev), &virtualio_fops) ;
	dev->dev.owner = THIS_MODULE ;
	dev->dev.ops = &virtualio_fops ;

	err = cdev_add(&(dev->dev), devno, 1) ;
	if(err){
		return err ;
	}


    dev->sem = semaphore_virtualio;
	dev->reg1 = 0 ;
	dev->reg2 = 0 ;
	dev->reg3 = 0 ;
	dev->reg4 = 0 ;

	return 0 ;
}

static int __init virtualio_init(void)
{
	int err = -1 ;
	dev_t dev = 0 ;
	struct device *temp = NULL ;

	printk(KERN_ALERT"Initializing virtualio device.\n") ;

	err = alloc_chrdev_region(&dev, 0, 1, VIRTUALIO_DEVICE_NODE_NAME) ;
	if(err < 0){
		printk(KERN_ALERT"Failed to alloc char dev region.\n") ;
		goto fail ;
	}

	virtualio_major = MAJOR(dev) ;
	virtualio_minor = MINOR(dev) ;

	virtualio_dev = kmalloc(sizeof(struct virtualio_android_dev), GFP_KERNEL) ;
	if(!virtualio_dev){
		err = -ENOMEM ;
		printk(KERN_ALERT"Failed to alloc virtualio_dev.\n") ;
		goto unregister ;
	}

	err = __virtualio_setup_dev(virtualio_dev) ;
	if(err){
		printk(KERN_ALERT"Failed to setup dev: %d\n", err) ;
		goto cleanup ;
	}

	virtualio_class = class_create(THIS_MODULE, VIRTUALIO_DEVICE_CLASS_NAME) ;
	if(IS_ERR(virtualio_class)){
		err = PTR_ERR(virtualio_class) ;
		printk(KERN_ALERT"Failed to create virtualio class.\n") ;
		goto destroy_cdev ;
	}

	temp = device_create(virtualio_class, NULL, dev, "%s", VIRTUALIO_DEVICE_FILE_NAME) ;
	if(IS_ERR(temp)){
		err = PTR_ERR(temp) ;
		printk(KERN_ALERT"Failed to create virtualio device.") ;
		goto destroy_class ;
	}

	dev_set_drvdata(temp, virtualio_dev) ;
	virtualio_create_proc() ;

	printk(KERN_ALERT"Succedded to initialize virtualio device.\n") ;
	return 0 ;

	device_destroy(virtualio_class, dev) ;

destroy_class:
	class_destroy(virtualio_class) ;

destroy_cdev:
	cdev_del(&(virtualio_dev->dev)) ;

cleanup:
	kfree(virtualio_dev) ;

unregister:
	unregister_chrdev_region(MKDEV(virtualio_major, virtualio_minor), 1) ;

fail:
	return err ;
}

static void __exit virtualio_exit(void) 
{
	dev_t devno = MKDEV(virtualio_major, virtualio_minor) ;
	printk(KERN_ALERT"Destroy virtualio device.\n") ;
	
	virtualio_remove_proc() ;

	if(virtualio_class){
		device_destroy(virtualio_class, MKDEV(virtualio_major, virtualio_minor)) ;
		class_destroy(virtualio_class) ;
	}

	if(virtualio_dev){
		cdev_del(&(virtualio_dev->dev)) ;
		kfree(virtualio_dev) ;
	}

	unregister_chrdev_region(devno, 1) ;

}

MODULE_LICENSE("GPL") ;
MODULE_DESCRIPTION("First android driver") ;

module_init(virtualio_init) ;
module_exit(virtualio_exit) ;