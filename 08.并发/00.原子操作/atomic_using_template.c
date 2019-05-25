//目的，使设备最多只能被一个进程打开
static atomic_t xxx_available = ATOMIC_INIT(1) ;//定义原子变量

static int xxx_open(struct inode *inode, struct file *filp)
{
    ...
    if(!atomic_dec_and_test(&xxx_available))
    {
        atomic_inc(&xxx_available) ;
        return -EBUSY ;//设备已经打开
    }
    ...
    return 0 ;//成功
}

static int xxx_release(struct inode *inode, struct file *filp)
{
    atomic_inc(&xxx_available) ;//释放设备
    return 0 ;
}