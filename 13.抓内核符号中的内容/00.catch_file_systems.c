#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#define FILE_SYSTEMS_ADDRESS 0xffffffff944f5f88
#define FILE_SYSTEMS_LOCK_ADDRESS 0xffffffff944f5f80

static int __init fsinfo_init(void){
    struct file_system_type *p, **f_head;
    rwlock_t *rwlock;

    rwlock = (rwlock_t *) FILE_SYSTEMS_LOCK_ADDRESS;
    f_head = (struct file_system_type **)FILE_SYSTEMS_LOCK_ADDRESS;

    printk("fsinfo init ................\n");
    
    read_lock(rwlock);
    f_head = (struct file_system_type **)FILE_SYSTEMS_LOCK_ADDRESS;
    for(p = *f_head; p; p = p->next){
        printk("file_system_type: %s\n", p->name);
    }
    read_unlock(rwlock);
    return 0;
}

static __exit void fsinfo_exit(void)
{
    printk(KERN_ALERT"fsinfo module exit\n") ;
}

module_init(fsinfo_init) ;
module_exit(fsinfo_exit) ;
