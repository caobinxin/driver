/*
    这里给出一个将内核空间某一物理页面映射到用户空间的例子。

    这里的物理页面其实可引申为设备的内存（比如某PCI-E显卡设备的frame buffer 所在的总线地址 0xd0000000）

    这个例子将展示用户空间如何通过mmap来映射某一段物理地址空间并对其进行操作。

    内核空间的物理页面通过　alloc_pages获得，其对应的物理地址将用printk打印出来，这样用户空间才可以告诉mmap函数要映射到那个物理页面上。
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/gfp.h>
#include <linux/string.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <asm/io.h>


#define KSTR_DEF "Hello world from kernel virtual space"

static struct cdev *pcdev ;
static dev_t ndev ;
static struct page *pg ;
static struct timer_list timer ;
static char *dev_buf_viraddr;

static void timer_func(unsigned long data)
{
    printk("timer_func: %s\n", (char *)data) ;
    timer.expires = jiffies + HZ*10 ;
    add_timer(&timer) ;
}

static int demo_open(struct inode *inode, struct file *filp)
{
    printk("demo_open return 0\n") ;
    return 0 ;
}

static int demo_release(struct inode *inode, struct file *filp)
{
    printk("demo_release return 0\n") ;
    return 0 ;
}

static int demo_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int err = 0 ;
    unsigned long start = vma->vm_start ;
    unsigned long size = vma->vm_end - vma->vm_start ;

    /*用remap_pfn_range将用户空间地址映射到内核空间的物理页面*/
    printk("demo_mmap vma->vm_pgoff = %d\n", vma->vm_pgoff) ;
    err = remap_pfn_range(vma, start, virt_to_phys((void *)dev_buf_viraddr)>>PAGE_SHIFT, size, vma->vm_page_prot) ;
    return err ;
}

static struct file_operations mmap_fops = 
{
    .owner = THIS_MODULE, 
    .open = demo_open,
    .release = demo_release,
    .mmap = demo_mmap,
} ;

static int demo_map_init(void)
{
    int err = 0 ;

    //模拟硬件内存
    /*在高端物理内存区（内核空间１GB 中高于　896m 的空间）分配　一个页面：*/
    pg = alloc_pages(GFP_HIGHUSER, 0) ;/*2^0 = 1 页*/
    /*设置页面的PG_reserved 属性，防止映射到用户空间的页面被　swap out 出去*/
    SetPageReserved(pg) ;
    /*因为物理页面来自高端内存，所以在使用前需要调用　kmap为该物理页面建立映射关系*/
    dev_buf_viraddr = (char *)kmap(pg) ;/*映射到高端映射区的虚拟地址*/

    strcpy(dev_buf_viraddr, KSTR_DEF) ;
    printk("内核空间中高端内存中页的物理地址　kpa = 0x%X, kernel string = %s\n", page_to_phys(pg), dev_buf_viraddr) ;

    pcdev = cdev_alloc() ;
    cdev_init(pcdev, &mmap_fops) ;
    alloc_chrdev_region(&ndev, 0, 1, "demo_mmap_dev") ;
    printk("major = %d, minor = %d\n", MAJOR(ndev), MINOR(ndev)) ;
    pcdev->owner = THIS_MODULE ;
    cdev_add(pcdev, ndev, 1) ;


    /*创建定时器每隔 10s 打印一次被映射的物理页面中的内容*/
    init_timer(&timer) ;
    timer.function = timer_func ;
    timer.data = (unsigned long)dev_buf_viraddr ;
    timer.expires = jiffies + HZ * 10 ;
    add_timer(&timer) ;

    return err ;
}

static void demo_map_exit(void)
{
    del_timer_sync(&timer) ;

    cdev_del(pcdev) ;
    unregister_chrdev_region(ndev, 1) ;

    kunmap(pg) ;
    ClearPageReserved(pg) ;
    __free_pages(pg, 0) ;
    
}

module_init(demo_map_init) ;
module_exit(demo_map_exit) ;

MODULE_AUTHOR("binxin cao") ;
MODULE_DESCRIPTION("A demo kernel module to remap a physical page to the user space") ;
MODULE_LICENSE("GPL") ;