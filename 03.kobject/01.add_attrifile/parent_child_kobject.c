#include <linux/init.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

MODULE_LICENSE("Dual BSD/GPL");

static struct kobject *parent = NULL ;
static struct kobject *child = NULL ;

static struct attribute cld_attr = {
    .name = "cld_attr",
    .mode = S_IRUGO | S_IWUSR,
} ;

static __init int cbx_kobject_init(void)
{
    parent = kobject_create_and_add("cbx_parent", NULL) ;
    child = kobject_create_and_add("cbx_child", parent);

    sysfs_create_file(child, &cld_attr) ;
    return 0;
}

static __exit void cbx_kobject_exit(void)
{
    sysfs_remove_file(child, &cld_attr);
    kobject_del(child) ;
    kobject_del(parent) ;
}

module_init(cbx_kobject_init) ;
module_exit(cbx_kobject_exit) ;