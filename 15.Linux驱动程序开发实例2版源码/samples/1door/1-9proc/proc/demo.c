#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PROC FILE DEMO");

#define MAX_LENGTH   1000
#define MAX_simple_LENGTH 1024

static struct proc_dir_entry *proc_entry,*proc_status;
static char *simple_buffer;

static int simple_show(struct seq_file *file, void *iter)
{
	struct task_struct *p;
	char state;

	seq_printf(file, "%5s%7s%7s%7s%7s%7s%7s  %s\n\n",
		"PID","UID","PRIO","POLICY",
		"STATE","UTIME","STIME","COMMAND");

	for_each_process(p) {
		int pid = p->pid;

		if (unlikely(!pid))
			continue;

		switch((int)p->state)
		{
		case -1: state='Z'; break;
		case 0: state='R'; break;
		default: state='S'; break;
		}

		seq_printf(file, "%5d%7d%7d%7d%7c%7d%7d  %s\n",
			(int)p->pid,
			(int)p->tgid,
			(int)p->rt_priority,
			(int)p->policy,
			state,
			(int)p->utime,
			(int)p->stime,
			p->comm);
	}

	return 0;
}

ssize_t simple_write( struct file *flip, const char __user *buff, size_t len, loff_t *offset)
{
	
	if(len>MAX_LENGTH)len=MAX_LENGTH;
	if (copy_from_user(simple_buffer, buff, len ))
	{
		return -EFAULT;
	}
	simple_buffer[len] = 0;
	printk(KERN_INFO "simple_write: %s\n",simple_buffer);
	return len;
}

static int simple_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, simple_show, inode->i_private);
}

static const struct file_operations proc_simple_fops = {
	.owner		= THIS_MODULE,
	.write		= simple_write,
	.read		= seq_read,
	.open		= simple_proc_open,
	.release	= single_release,
};

int init_simple_module( void )
{
	int ret = 0;
	simple_buffer = (char *)vmalloc( MAX_simple_LENGTH );
	if (!simple_buffer) 
	{
		ret = -ENOMEM;
	} 
	else 
	{
		memset( simple_buffer, 0, MAX_LENGTH );
		proc_entry=proc_mkdir("demo", NULL);
		proc_status = proc_create("status", 0,proc_entry,&proc_simple_fops);
		if (!proc_status)
		{
			ret = -ENOMEM;
			vfree(simple_buffer);
			printk(KERN_INFO "demo: Couldn't create proc entry\n");
			
		} 
		else
		{
			printk(KERN_INFO "demo: Module loaded.\n");
		}
	}
	return ret;
}


void cleanup_simple_module( void )
{
	proc_remove(proc_entry);
	vfree(simple_buffer);
	printk(KERN_INFO "demo: Module unloaded.\n");
}


module_init( init_simple_module );
module_exit( cleanup_simple_module );

