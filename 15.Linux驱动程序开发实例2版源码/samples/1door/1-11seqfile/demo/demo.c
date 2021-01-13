
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/seq_file.h>

#include "demo.h"

MODULE_AUTHOR("fgj");
MODULE_LICENSE("Dual BSD/GPL");

struct simple_dev *simple_devices;
static unsigned char simple_inc=0;

struct simple_record
{
	struct list_head list;
	char name[8];
	int iflag;
};

static struct list_head g_seqfile;

static int simple_seq_show(struct seq_file *m, void *v)
{
        struct simple_record *sr = list_entry(v, struct simple_record, list);
	printk("simple_seq_show %d\n",sr->iflag);
        seq_printf(m, "name: %s, iflag:%d\n", sr->name,sr->iflag);
        return 0;
}

static void *simple_seq_start(struct seq_file *m, loff_t *pos)
{
	struct list_head *lh=seq_list_start(&g_seqfile, *pos);
	printk("simple_seq_start *pos=%.8x\n",*pos);
        return lh;
}

static void *simple_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	printk("simple_seq_next *pos=%.8x\n",*pos);
        return seq_list_next(v, &g_seqfile, pos);
}

static void simple_seq_stop(struct seq_file *m, void *v)
{
	printk("simple_seq_stop\n");
}

static struct seq_operations simple_seq_ops = {
        .start = simple_seq_start,
        .next = simple_seq_next,
        .stop = simple_seq_stop,
        .show = simple_seq_show
};

int simple_open(struct inode *inode, struct file *filp)
{
	struct simple_dev *dev;
	if(simple_inc>0)return -ERESTARTSYS;
	simple_inc++;
	dev = container_of(inode->i_cdev, struct simple_dev, cdev);

	return seq_open(filp, &simple_seq_ops);
}

int simple_release(struct inode *inode, struct file *filp)
{
	simple_inc--;
	return 0;
}

struct file_operations simple_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read =	seq_read,
    	.llseek = seq_lseek,
    	.release = simple_release
};

/*******************************************************
                MODULE ROUTINE
*******************************************************/
void simple_cleanup_module(void)
{
	dev_t devno = MKDEV(simple_MAJOR, simple_MINOR);

	if (simple_devices) 
	{
		cdev_del(&simple_devices->cdev);
		kfree(simple_devices);
	}
	unregister_chrdev_region(devno,1);
}

int simple_init_module(void)
{
	int result;
	dev_t dev = 0;
	int i=0;
	
	INIT_LIST_HEAD(&g_seqfile);
	printk("&g_seqfile=%.8x\n",&g_seqfile);
	for(i=0;i<4;i++)
	{
		struct simple_record*sr;
		sr=kmalloc(sizeof(struct simple_record),GFP_KERNEL);
		if(sr==NULL) return -1;
		memset(sr->name,0,4);
		sprintf(sr->name,"sr%d",4-i);
		sr->iflag=4-i;
		list_add(&sr->list, &g_seqfile);
		printk("&sr->list[%d]=%.8x\n",i,&sr->list);
	}
	struct list_head *pos;
	list_for_each(pos,&g_seqfile)
        {
              	struct simple_record*p=list_entry(pos, struct simple_record,list);
		printk("initfind the %d list element\n",p->iflag);
        }

	dev = MKDEV(simple_MAJOR, simple_MINOR);
	result = register_chrdev_region(dev, 1, "DEMO");
	if (result < 0) 
	{
		printk(KERN_WARNING "DEMO: can't get major %d\n", simple_MAJOR);
		return result;
	}

	simple_devices = kmalloc(sizeof(struct simple_dev), GFP_KERNEL);
	if (!simple_devices)
	{
		result = -ENOMEM;
		goto fail;
	}
	memset(simple_devices, 0, sizeof(struct simple_dev));

	cdev_init(&simple_devices->cdev, &simple_fops);
	simple_devices->cdev.owner = THIS_MODULE;
	simple_devices->cdev.ops = &simple_fops;
	result = cdev_add (&simple_devices->cdev, dev, 1);
	if(result)
	{
		printk(KERN_NOTICE "Error %d adding DEMO\n", result);
		goto fail;
	}
	return 0;
fail:
	simple_cleanup_module();
	return result;
}

module_init(simple_init_module);
module_exit(simple_cleanup_module);
