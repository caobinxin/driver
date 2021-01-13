
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
#include <linux/mm.h>
#include <linux/ioctl.h>

MODULE_DESCRIPTION("mmap demo driver");
MODULE_LICENSE("Dual BSD/GPL");

#define mmapmem_MAJOR 224
#define mmapmem_MINOR 0
#define MAP_BUFFER_SIZE (1920*1080*2)

struct mmapmem_dev 
{
	struct cdev cdev;
};

struct mmapmem_dev *mmapmem_devices;
static unsigned char mmapmem_inc=0;
static char*buffer=NULL;
static char*buffer_area=NULL;

int mmapmem_open(struct inode *inode, struct file *filp)
{
	struct mmapmem_dev *dev;
	mmapmem_inc++;

	dev = container_of(inode->i_cdev, struct mmapmem_dev, cdev);
	filp->private_data = dev;

	return 0;
}

int mmapmem_release(struct inode *inode, struct file *filp)
{
	mmapmem_inc--;
	return 0;
}

static int mmapmem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret;
	ret = remap_pfn_range(vma,
		   vma->vm_start,
		   virt_to_phys((void*)((unsigned long)buffer_area)) >> PAGE_SHIFT,
		   vma->vm_end-vma->vm_start,
		   PAGE_SHARED);
	if(ret != 0) {
		return -EAGAIN;
	}
	return 0;
}

struct file_operations mmapmem_fops = {
	.owner =    THIS_MODULE,
	.open =     mmapmem_open,
	.mmap =    mmapmem_mmap,
	.release =  mmapmem_release,
};

void mmapmem_cleanup_module(void)
{
	dev_t devno = MKDEV(mmapmem_MAJOR, mmapmem_MINOR);
	unsigned long virt_addr;
	if (mmapmem_devices) 
	{
		cdev_del(&mmapmem_devices->cdev);
		kfree(mmapmem_devices);
	}
	for(virt_addr=(unsigned long)buffer_area; virt_addr<(unsigned long)buffer_area+MAP_BUFFER_SIZE;
	virt_addr+=PAGE_SIZE)
	{
		 SetPageReserved(virt_to_page(virt_addr));
	}
	if (buffer)
		kfree(buffer);
	unregister_chrdev_region(devno,1);
}

int mmapmem_init_module(void)
{
	int result;
	dev_t dev = 0;
	unsigned long virt_addr;
	
	dev = MKDEV(mmapmem_MAJOR, mmapmem_MINOR);
	result = register_chrdev_region(dev, 1, "mmapmem");
	if (result < 0) 
	{
		printk(KERN_WARNING "mmapmem: can't get major %d\n", mmapmem_MAJOR);
		goto out_free;
	}

	mmapmem_devices = kmalloc(sizeof(struct mmapmem_dev), GFP_KERNEL);
	if (!mmapmem_devices)
	{
		result = -ENOMEM;
		goto out_free;
	}
	memset(mmapmem_devices, 0, sizeof(struct mmapmem_dev));

	cdev_init(&mmapmem_devices->cdev, &mmapmem_fops);
	mmapmem_devices->cdev.owner = THIS_MODULE;
	mmapmem_devices->cdev.ops = &mmapmem_fops;
	result = cdev_add (&mmapmem_devices->cdev, dev, 1);
	if(result)
	{
		printk(KERN_NOTICE "mmapmem:Error %d adding mmapmem\n", result);
		goto out_free;
	}
	
	buffer = kmalloc(MAP_BUFFER_SIZE,GFP_KERNEL);            
        printk(" mmap buffer = %p\n",buffer);            
	buffer_area=(int *)(((unsigned long)buffer + PAGE_SIZE -1) & PAGE_MASK);

	for (virt_addr=(unsigned long)buffer_area; virt_addr<(unsigned long)buffer_area+MAP_BUFFER_SIZE;virt_addr+=PAGE_SIZE)
	{
		/* reserve all pages to make them remapable */
		SetPageReserved(virt_to_page(virt_addr));
	}
	memset(buffer,0,MAP_BUFFER_SIZE);
	return 0;

out_free:
	mmapmem_cleanup_module();
	return result;
}


module_init(mmapmem_init_module);
module_exit(mmapmem_cleanup_module);
