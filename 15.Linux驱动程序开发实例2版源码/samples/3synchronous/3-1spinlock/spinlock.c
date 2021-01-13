
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");

#define MAJOR_NUM 224

static ssize_t simplespin_read(struct file *, char *, size_t, loff_t*);
static ssize_t simplespin_write(struct file *, const char *, size_t, loff_t*);
static int simplespin_open(struct inode *inode, struct file *filp);
static int simplespin_release(struct inode *inode, struct file *filp);

struct file_operations simplespin_fops =
{
	.read=simplespin_read, 
	.write=simplespin_write, 
	.open=simplespin_open,
	.release=simplespin_release,
};

static int simplespin_var = 0;
static int simplespin_count = 0;
static spinlock_t spin;

static int simplespin_open(struct inode *inode, struct file *filp)
{
	/*获得自选锁*/
	spin_lock(&spin);
	
	/*临界资源访问*/
	if (simplespin_count)
	{
		spin_unlock(&spin);
		return - EBUSY;
	}
	simplespin_count++;
	
	/*释放自选锁*/
	spin_unlock(&spin);
	return 0;
}

static int simplespin_release(struct inode *inode, struct file *filp)
{
	simplespin_count--;
	return 0;
}

static ssize_t simplespin_read(struct file *filp, char *buf, size_t len, loff_t*off)
{
	return 0;
}

static ssize_t simplespin_write(struct file *filp, const char *buf, size_t len,loff_t *off)
{
	return 0;
}

static int __init simplespin_init(void)
{
	int ret;
	spin_lock_init(&spin);
	/*注册设备驱动*/
	ret = register_chrdev(MAJOR_NUM, "chardev", &simplespin_fops);
	if (ret)
	{
		printk("chardev register failure\n");
	}
	else
	{
		printk("chardev register success\n");
	}
	return ret;
}

static void __exit simplespin_exit(void)
{
	/*注销设备驱动*/
	unregister_chrdev(MAJOR_NUM, "chardev");
}

module_init(simplespin_init);
module_exit(simplespin_exit); 
