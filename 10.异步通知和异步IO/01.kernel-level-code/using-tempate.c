struct xxx_dev{
    struct cdev cdev ;
    ...
    struct fasync_struct *async_queue ;
}

static int xxx_fasync(int fd, struct file *filp, int mode)
{
    struct xxx_dev *dev = filp->private_data ;
    return fasync_helper(fd, filp, mode, &dev->async_queue) ;
}

static ssize_t xxx_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct xxx_dev *dev = filp->private_data ;
    ...

    /*产生异步读信号*/
    if(dev->async_queue){
        kill_fasync(&dev->async_queue, SIGIO, POLL_IN) ;
    }

    ...
}

static int xxx_release(struct inode *inode, struct file *filp)
{
    /*将文件从异步通知列表中删除*/
    xxx_fasync(-1, filp, 0) ;
    ...

    return 0 ;
}