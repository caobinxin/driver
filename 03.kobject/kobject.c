#include <linux/init.h>
#include <linux/module.h>
#include <linux/kobject.h>

MODULE_LICENSE("Dual BSD/GPL");

static struct kobject *parent = NULL ;

static __init int cbx_kobject_init(void)
{
    parent = kobject_create_and_add("cbx_obj", NULL) ;
    return 0;
}

static __exit void cbx_kobject_exit(void)
{
   kobject_del(parent) ;
}

module_init(cbx_kobject_init) ;
module_exit(cbx_kobject_exit) ;