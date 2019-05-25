//使用信号量实现设备只能被一个进程打开

static DECLARE_MUTEX(xxx_lock) ;

static int xxx_open(struct inode *inode, struct file *filp)
{
    ...
    if(down_trylock(&xxx_lock))//获得打开锁
    {
        return -EBUSY ;//设备忙
    }

    ...
    return 0 ;
}

static int xxx_release(struct inode *inode, struct file *filp)
{
    up(&xxx_lock) ;//释放锁
    return 0 ;
}