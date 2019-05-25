#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL") ;

static char *my_name = "colby";
static int num = 5000 ;

static __init int person_init(void)
{
    printk(KERN_INFO"persion name: %s\n", my_name) ;
    printk(KERN_INFO"persion num: %d\n", num) ;

    return 0;
}

static __exit void persion_exit(void)
{
    printk(KERN_ALERT"persion module exit\n") ;
}

module_init(person_init) ;
module_exit(persion_exit) ;
module_param(num, int, S_IRUGO) ;
module_param(my_name, charp, S_IRUGO) ;

MODULE_AUTHOR("colby") ;
MODULE_DESCRIPTION("带参数的模块测试") ;
MODULE_VERSION("V1.0") ;