int xxx_count = 0 ;//定义文件打开次数计数
spinlock_t lock ;


static int xxx_open(struct inode *inode, struct file *filp)
{
    ...
    spin_lock(&xxx_lock) ;
    if(xxx_count)//已经打开
    {
        spin_unlock(&xxx_lock) ;
        return -EBUSY ;
    }
    xxx_count++ ;
    spin_unlock(&xxx_lock) ;
    ...

    return 0 ;//成功
}

static int xxx_release(struct inode *inode, struct file *filp)
{
    ...
    spin_lock(&xxx_lock) ;
    xxx_count-- ;
    spin_unlock(&xxx_lock) ;
    ...
    
    return 0 ;//成功
}

void xxx_probe(...)
{
    ...
    spin_lock_init(&lock) ;
    ...
}