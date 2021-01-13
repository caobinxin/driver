#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/phy.h>

#include <linux/list.h>

struct buffer_head_test{
	int iflag;
	struct list_head bh_list;
};

struct buffer_head_test a;
static int __init list_head_init(void)  
{  
        struct buffer_head_test * bhg=NULL;
        struct buffer_head_test * p=NULL;
	struct list_head *pos;
	int i=0;

	INIT_LIST_HEAD(&a.bh_list);
	for(i=0;i<5;i++)
	{
		bhg=kmalloc(sizeof(struct buffer_head_test),GFP_KERNEL);
		if(bhg==NULL) return -1;
		bhg->iflag=i;
		list_add(&bhg->bh_list,&a.bh_list);
	}
	printk("list_head_init ok\n");
	list_for_each(pos,&a.bh_list)
        {
              	p=list_entry(pos, struct buffer_head_test, bh_list);
		printk("initfind the %d list element\n",p->iflag);
        }

	return 0;
}  
  
static void __exit list_head_exit(void)  
{  
        struct buffer_head_test * p=NULL;
	struct buffer_head_test * from,*scratch;
	struct list_head *pos;
	list_for_each_entry_safe(from,scratch,&a.bh_list,bh_list)
        {
		printk("del the %d list element\n",from->iflag);
		list_del(&from->bh_list);
	        kfree(from);
        }

	list_for_each(pos,&a.bh_list)
        {
              	p=list_entry(pos, struct buffer_head_test, bh_list);
		printk("delfind the %d list element\n",p->iflag);
        }
}  
  
module_init(list_head_init);  
module_exit(list_head_exit); 

MODULE_AUTHOR("Grimm Feng");  
MODULE_DESCRIPTION("listhead test");  
MODULE_LICENSE("GPL"); 

