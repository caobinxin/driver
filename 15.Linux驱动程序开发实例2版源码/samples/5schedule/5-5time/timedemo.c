
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>	
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/pagemap.h>
#include <linux/times.h>

#include "demo.h"

MODULE_AUTHOR("fgj");
MODULE_LICENSE("Dual BSD/GPL");

#define SIMPLE_TIMER_DELAY 2*HZ//2Second

struct simple_dev *simple_devices;
static unsigned char simple_inc=0;
struct timeval start,stop,diff; 
static struct timer_list simple_timer;
static void simple_timer_handler(unsigned long data);

int timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y) 
{ 
    if(x->tv_sec>y->tv_sec) 
      return -1; 
  
    if((x->tv_sec==y->tv_sec)&&(x->tv_usec>y->tv_usec)) 
      return -1; 
  
    result->tv_sec = ( y->tv_sec-x->tv_sec ); 
    result->tv_usec = ( y->tv_usec-x->tv_usec ); 
  
    if(result->tv_usec<0) 
    { 
      result->tv_sec--; 
      result->tv_usec+=1000000; 
    } 
    return 0; 
} 

static void simple_timer_handler( unsigned long data)
{
    do_gettimeofday(&stop); 
    timeval_subtract(&diff,&start,&stop); 
    printk("%d S %d MS elapsed\n",diff.tv_sec,diff.tv_usec); 
	mod_timer(&simple_timer, jiffies + HZ);  
	return ;
}

/*******************************************************
      MODULE ROUTINE
*******************************************************/
void simple_cleanup_module(void)
{
	del_timer(&simple_timer);
}

int simple_init_module(void)
{
	int result;

	/* Register timer */
	init_timer(&simple_timer);
	simple_timer.function = &simple_timer_handler;
	simple_timer.expires = jiffies + SIMPLE_TIMER_DELAY;
	add_timer (&simple_timer);
	do_gettimeofday(&start); 
	return 0; /* succeed */
}

module_init(simple_init_module);
module_exit(simple_cleanup_module);
