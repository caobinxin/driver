static unsigned int xxx_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0 ;
    struct xxx_dev *dev = filp->private_data ;

    ...
    poll_wait(filp, &dev->r_wait, wait) ;//加入读等待队列
    poll_wait(filp, &dev->w_wait, wait) ;//加入写等待队列

    if(...){
        mask |= POLLIN | POLLRDNORM ;//标示数据可获得（对用户可读）
    }

    if(...){
        mask |= POLLOUT | POLLWRNORM ;//标示数据可写
    }
    ...

    return mask ;
}