#include <linux/init.h>  
#include <linux/module.h>  
#include <linux/sysctl.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>

static int __init filerw_module_init(void)  
{  
	struct file *fd=filp_open("/home/fs.txt", O_RDWR | O_NDELAY, 0);
	if (IS_ERR(fd))
	{
		printk("open /home/fs.txt failed\n");
		return -1;
	}
	else
	{
		ssize_t len=0;
		printk("open fs.txt successfully\n");
		char buffertmp[4];
		loff_t pos=0;

		mm_segment_t oldfs = get_fs();
		set_fs(KERNEL_DS);//let kernel buffer to be used in vfs_write/read

		memset(buffertmp,0,4);
		buffertmp[0]=0x31;
		len=vfs_write(fd,buffertmp,1,&pos);
		printk("write buffertmp=%s,len=%d\n",buffertmp,len);

		memset(buffertmp,0,4);
		pos=0;
		vfs_read(fd,buffertmp,1,&pos);
		printk("read buffertmp=%s\n",buffertmp);

		set_fs(oldfs);

		filp_close(fd,NULL);
	}
	return 0;  
}  

static void __exit filerw_module_exit(void)  
{  
	return;
}  

module_init(filerw_module_init);  
module_exit(filerw_module_exit);  
MODULE_AUTHOR("fgj");  
MODULE_LICENSE("GPL");  

