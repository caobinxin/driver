struct globalfifo_dev{
    struct cdev cdev ;
    unsigned int current_len ;
    unsigned char mem[GLOBALFIFO_SIZE] ;
    struct mutex mutex ;
    wait_queue_head_t r_wait ;
    wait_queue_head_t w_wait ;
    struct fasync_struct *async_queue ;
} ;

static int globalfifo_fasync(int fd, struct file *filp, int mode)
{
    struct globalfifo_dev *dev = filp->private_data ;
    return fasync_helper(fd, filp, mode, &dev->async_queue) ;
}

/**
 * 在globalfifo设备被正确写入之后，它变得可读，这个时候驱动应该释放SIGIO信号，以便应用程序捕获，
 * 
*/

static ssize_t globalfifo_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    struct globalfifo_dev *dev = filp->private_data ;
    int ret ;
    DECLARE_WAITQUEUE(wait, current) ;

    mutex_lock(&dev->mutex) ;
    add_wait_queue(&dev->w_wait, &wait) ;

    while(dev->current_len == GLOBALFIFO_SIZE){
        if( filp->f_flags & O_NONBLOCK){
            ret = -EAGAIN ;
            goto out ;
        }
        __set_current_state(TASK_INTERRUPTIBLE) ;
        mutex_unlock(&dev->mutex) ;

        schedule() ;
        if( signal_pending(current)){
            ret = -ERESTARTSYS ;
            goto out2 ;
        }

        mutex_lock(&dev->mutex) ;
    }

    if(count > GLOBALFIFO_SIZE - dev->courrent_len){
        count = GLOBALFIFO_SIZE - dev->current_len ;
    }

    if(copy_from_user(dev->mem + dev->current_len, buf, count)){
        ret = -EFAULT ;
        goto out ;
    }else{
        dev->current_len += count ;
        printk(KERN_INFO"written %d bytes(s), current_len:%d\n", count, dev->current_len) ;

        wake_up_interruptible(&dev->r_wait) ;

        if(dev->async_queue){
            kill_fasync(&dev->async_queue, SIGIO, POLL_IN) ;
            printk(KERN_DEBUG"%s kill SIGIO\n", __func__) ;
        }

        ret = count ;
    }

out:
    mutex_unlock(&dev->mutex) ;
out2:
    remove_wait_queue(&dev->w_wait, &wait) ;
    __set_current_state(TASK_RUNNING) ;
    return ret ;
}

/**
 * 增加异步通知后的globalfifo设备驱动的release()函数中需要调用globalfifo_fasync()函数将文件从异步通知列表中删除，
 * 
*/

static int globalfifo_release(struct inode *inode, struct file *filp)
{
    globalfifo_fasync(-1, filp, 0) ;
    return 0 ;
}

