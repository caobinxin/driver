#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");

int add_integar(int a, int b)
{
    return a + b;
}

int sub_integar(int a, int b)
{
    return a -b ;
}

EXPORT_SYMBOL(add_integar);
EXPORT_SYMBOL(sub_integar);