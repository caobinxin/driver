/*在进行IO操作的时候，判断设备是否可写，如果不可写且为阻塞IO,则进程睡眠并挂起到等待队列中*/

static ssize_t xxx_write(struct file *file, const char *buffer, size_t count, loff_t *ppos)
{
    ...
    DECLARE_WAITQUEUE(wait, current) ;//定义等待队列元素 current代表的就是当前的进程
    add_wait_queue(&xxx_wait, &wait) ;//添加元素到等待队列中

    /*等待设备缓存区可写*/
    do{
        avail = device_writable(...) ;
        if(avail < 0){
            if(file->f_flags & O_NONBLOCK){/*非阻塞的处理*/
                ret = -EAGAIN ;
                goto out ;
            }

            __set_current_state(TASK_INTERRUPTIBLE) ;/*改变进程状态*/
            schedule() ;/*调度其他进程执行*/
            if(signal_pending(current)){/*如果是因为信号唤醒*/
                ret = -ERESTARTSYS ;
                goto out ;

            }
        }
    }while(avail < 0);

    /*写设备缓冲区*/
    device_writable(...) ;
    
out:
    remove_wait_queue(&xxx_wait, &wait) ;/*将元素移出　xxx_wait指引的队列*/
    __set_current_state(TASK_RUNNING) ;/*设置进程状态为TASK_RUNNING*/
    
    return ret ;
    
}