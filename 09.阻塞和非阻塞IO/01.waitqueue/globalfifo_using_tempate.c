struct globalfifo_dev{
    struct cdev cdev ;
    unsigned int current_len ;//表征当前FIFO中有效数据的长度。current_len==0 意味者FIFO空， current_len == GLOBALFIFO_SIZE 意味者FIFO满
    unsigned char mem[GLOBALFIFO_SIZE] ;
    struct mutex mutex ;
    wait_queue_head_t r_wait ;//等待队列的头
    wait_queue_head_t w_wait ;//等待队列的头
} ;

static int __init globalfifo_init(void)
{
    int ret ;
    dev_t devno = MKDEV(globalfifo_major, 0) ;

    if(globalfifo_major){
        ret = register_chrdev_region(devno, 1, "globalfifo") ;
    }else{
        ret = alloc_chrdev_region(&devno, 0, 1, "globalfifo") ;
        globalfifo_major = MAJOR(devno) ;
    }

    if(ret < 0){
        return ret ;
    }

    globalfifo_devp = kzalloc(sizeof(struct globalfifo_dev), GFP_KERNEL) ;
    if(!globalfifo_devp){
        ret = -ENOMEM ;
        goto fail_malloc ;
    }

    globalfifo_setup_cdev(globalfifo_devp, 0) ;

    mutex_init(&globalfifo_devp->mutex) ;
    init_waitqueue_head(&globalfifo_devp->r_wait) ;//初始化等待队列头
    init_waitqueue_head(&globalfifo_devp->w_wait) ;

    return 0 ;
fail_malloc:
    unregister_chrdev_region(devno, 1) ;
    return ret ;
}

module_init(globalfifo_init) ;



static ssize_t globalfifo_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    int ret ;
    struct globalfifo_dev *dev = filp->private_data ;
    DECLARE_WAITQUEUE(wait, current) ;//定义等待队列元素 current代表的就是当前的进程

    mutex_lock(&dev->mutex) ;
    add_wait_queue(&dev->r_wait, &wait) ;//加入等待队列中

    while(dev->current_len == 0){
        if(filp->f_flags & O_NONBLOCK){
            ret = -EAGAIN ;
            goto out ;
        }
        __set_current_state(TASK_INTERRUPTIBLE) ;
        mutex_unlock(&dev->mutex) ;

        schedule() ;
        if(signal_pending(current)){
            ret = -ERESTARTSYS ;
            goto out2 ;
        }

        mutex_lock(&dev->mutex) ;
    }

    if(count > dev->current_len){
        count = dev->current_len ;
    }

    if(copy_to_user(buf, dev->mem, count)){
        ret = -EFAULT ;
        goto out ;
    }else{
        memcpy(dev->mem, dev->mem + count, dev->current_len - count) ;
        dev->current_len -= count ;
        printk(KERN_INFO"read %d bytes(s), current_len:%d\n", count, dev->current_len) ;

        wake_up_interruptible(&dev->w_wait) ;

        ret = count ;
    }

out:
    mutex_unlock(&dev->mutex) ;

out2:
    remove_wait_queue(&dev->w_wait, &wait) ;
    __set_current_state(TASK_RUNNING) ;
    return ret ;
}

static ssize_t globalfifo_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    struct globalfifo_dev *dev = filp->private_data ;
    int ret ;

    DECLARE_WAITQUEUE(wait, current) ;
    mutex_lock(&dev->mutex) ;
    add_wait_queue(&dev->w_wait, &wait) ;

    while(dev->current_len == GLOBALFIFO_SIZE){
        if(filp->f_flags & O_NONBLOCK){
            ret = -EAGAIN ;
            goto out ;
        }
        __set_current_state(TASK_INTERRUPTIBLE) ;

        mutex_unlock(&dev->mutex) ;
        schedule() ;
        if(signal_pending(current)){
            ret = -ERESTARTSYS ;
            goto out2 ;
        }

        mutex_lock(&dev->mutex) ;
    }

    if(count > GLOBALFIFO_SIZE - dev->current_len){
        count = GLOBALFIFO_SIZE - dev->current_len ;
    }

    if(copy_from_user(dev->mem + dev->current_len, buf, count)){
        ret = -EFAULT ;
        goto out ;
    }else{
        dev->current_len += count ;
        printk(KERN_INFO"written %d bytes(s), current_len:%d\n", count, dev->current_len) ;

        wake_up_interruptible(&dev->r_wait) ;
        
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
 * 
 *  这里有一个关键点，无论读函数还是写函数，在进入真正的读写之前，都要再次判断设备是否可以读写，　
 * 
 * 　while(dev->current_len == 0){
 * 
 *　　while(dev->current_len == GLOBALFIFO_SIZE){
 * 
 *  主要的目的是为了防止并发的读或者并发的写都正确。设想如果两个读进程都阻塞在读上，写进程执行的wake_up_interruptible(&dev->r_wait)实际会同时唤醒它们
 * 其中先执行的那个进程可能会率先将FIFO再次读空。
 * 
*/




