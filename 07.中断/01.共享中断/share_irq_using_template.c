/*中断处理顶半部*/

irqreturn_t xxx_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    ...
    int status = read_int_status() ;/*获知中断源*/
    if(!is_myint(dev_id, status))/*判断是否是本设备中断*/
    {
        return IRQ_NONE ;/*立即返回*/
    }
    ...

    return IRQ_HANDLED ;
}

/*设备驱动模块加载函数*/
int xxx_init(void)
{
    ...
    /*申请共享中断*/
    result = request_irq( sh_irq, xxx_interrupt, SA_SHIRQ, "xxx", xxx_dev) ;
    ...
}

/*设备驱动模块卸载函数*/
void xxx_exit(void)
{
    ...
    /*释放中断*/
    free_irq(xxx_irq, xxx_interrupt) ;
    ...
}