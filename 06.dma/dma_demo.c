/*设备结构体*/
typedef struct
{
   void *dma_buffer ; //dma缓冲区

   /*当前dma的相关信息*/
   struct{
       unsigned int direction ;//方向
       unsigned int length ;
       void *target ;//目标
       unsigned long start_time ;//开始时间
   }current_dma ;

   unsigned char dma ;//dma通道
}xxx_device ;

static int xxx_open(...)
{
    ...
    /*安装中断服务程序*/
    if( ( retval = request_irq(dev->irq, &xxx_interrupt, 0, dev->name, dev)))
    {
        printk(KERN_ERR"%s: could not allocate IRQ%d\n", dev->name, dev->irq) ;
        return retval ;
    }

    /*申请DMA*/
    if( (retval = request_dma(dev->dma, dev->name))){
        free_irq(dev->irq, dev) ;
        printk(KERN_ERR"%s: could not allocate DMA%d channel\n", ...) ;
        return retval ;
    }

    /*申请dma缓冲区*/
    dev->dma_buffer = (void *) dma_mem_alloc( DMA_BUFFER_SIZE) ;
    if( !dev->dma_buffer)
    {
        printk(KERN_ERR"%s: could not allocate DMA buffer\n", dev->name) ;
        free_dma(dev->dma) ;
        free_irq(dev->irq, dev) ;
        return -ENOMEM ;
    }

    /*初始化DMA*/
    init_dma() ;
}

/*内存到外设*/
static int mem_to_xxx(const byte *buf, int len)
{
    ...
    dev->current_dma.direction = 1 ;/*dma方向*/
    dev->current_dma.start_time = jiffies ;/*记录dma开始的时间*/

    memcpy(dev->dma_buffer, buf, len) ;
    target = isa_virt_to_bus(dev->dma_buffer) ;/*假设xxx挂接在ISA总线上*/

    /*进行一次dma写操作*/
    flags = claim_dma_lock() ;
    disable_dma(dev->dma) ;/*禁止dma*/
    clear_dma_ff(dev->dma) ;/*清除dma flip-flop*/
    set_dma_mode(dev->dma, 0x48) ;/* dma内存 -> io*/
    set_dma_addr(dev->dma, target) ;/*设置dma地址*/
    set_dma_count(dev->dma, len) ;/*设置dma长度*/
    outb_control(dev->x_ctrl | DMAE | TCEN, dev) ;/*让设备接受dma*/
    enable_dma(dev->dma) ;/*使能dma*/
    release_dma_lock(flags) ;

    printk(KERN_DEBUG"%s: dma transfer started\n", dev->name) ;
    ...
}

/*外设到内存*/
static void xxx_to_mem(const char *buf, int len, char *target)
{
    ...
    /*记录dma信息*/
    dev->current_dma.target = target ;
    dev->current_dma.direction = 0 ;
    dev->current_dma.start_time = jiffies ;
    dev->current_dma.length = len ;

    /*进行一次dma读操作*/
    outb_control(dev->x_ctrl | DIR | TCEN | DMAE, dev) ;
    flags = claim_dma_lock() ;
    disable_dma(dev->dma) ;
    clear_dma_ff(dev->dma) ;
    clear_dma_ff(dev->dma) ;
    set_dma_mode(dev->dma, 0x04) ;/*ＩＯ->mem*/
    set_dma_addr(dev->dma, isa_virt_to_bus(target)) ;
    set_dma_count(dev->dma, len) ;
    enable_dma(dev->dma) ;
    release_dma_lock(flags) ;
    ...
}

/*设备中断处理*/
static irqreturn_t xxx_interrupt(int irq, void *dev_id, struct pt_regs *reg_ptr)
{
    ...
    do{
        /*dma传输完成*/
        if(int_type == DMA_DONE)
        {
            outb_control(dev->x_ctrl &~( DMAE | TCEN | DIR), dev) ;
            if( dev->current_dma.direction)
            {
                /* 内存 -> ＩＯ*/
            }else{
                /*ＩＯ　-> 内存*/

                memcpy(dev->current_dma.target, dev->dma_buffer, dev->current_dma.length) ;
            }
        }else if( int_type == RECV_DATA)/*接受到数据*/{
            xxx_to_mem(...) ; /**通过dma读接收的数据到内存/
        }
    }
}
