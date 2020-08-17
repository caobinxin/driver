
/*测试说明：
 * 测试	dma和  接收中断和发送中断
 * 
 */
 #include "m6x_api.h"
#define DRVER_NAME      "m6x"
typedef struct {
	resource_size_t mmio_start;
	resource_size_t mmio_end;
	resource_size_t mmio_len;
	unsigned long mmio_flags;
	unsigned char *mem;
	unsigned char *vmem;
} bar_info_t;

typedef struct {
	struct pci_dev *dev;
	struct pci_saved_state *save;
	int legacy_en; /*legacy 遗产*/
	int msi_en;
	int msix_en;
	int in_use;
	int irq;
	int irq_num;
	int irq_en;
	int bar_num;
	bar_info_t bar[8];
	struct msix_entry msix[100];
	/* board info */
	unsigned char revision;
	unsigned char irq_pin;
	unsigned char irq_line;
	unsigned short sub_vendor_id;
	unsigned short sub_system_id;
	unsigned short vendor_id;
	unsigned short device_id;
} m6x_pcie_t;
m6x_pcie_t *m6x_pcie_device = NULL;


int hexdump(char *name, char *buf, int len)
{
	int i, count;
	unsigned int *p;
	count = len / 32;
	count += 1;
	printk(KERN_ERR "hexdump %s mem:0x%lx len:%d\n", name, virt_to_phys(buf), len);
	for (i = 0; i < count; i++) {
		p = (unsigned int *)(buf + i * 32);
		printk(KERN_ERR
		       "mem[0x%04x] 0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,\n",
		       i * 32, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
	}
	return 0;
}


int dmalloc(dm_t *dm, int size)
{
	struct device *dev = &(m6x_pcie_device->dev->dev);
	
	if (NULL == dev) {
		printk(KERN_ERR "%s(NULL)\n", __func__);
		return -1;
	}
	if (dma_set_mask(dev, DMA_BIT_MASK(64))) {
		printk(KERN_ERR "dma_set_mask err\n");
		if (dma_set_coherent_mask(dev, DMA_BIT_MASK(64))) {
			printk(KERN_ERR "dma_set_coherent_mask err\n");
			return -1;
		}
	}
	dm->vir =
	    (unsigned long)dma_alloc_coherent(dev, size,
					      (dma_addr_t *)(&(dm->phy)), GFP_DMA);
	if (dm->vir == 0) {
		printk(KERN_ERR "dma_alloc_coherent err\n");
		return -1;
	}
	dm->size = size;
	//memset((unsigned char *)(dm->vir), 0x56, size);
	printk(KERN_ERR "dma_alloc_coherent(%d) 0x%lx 0x%lx\n", size, dm->vir, dm->phy);
	return 0;
}

int hwcopy(unsigned char *dest, unsigned char *src, int len)
{
	int i;
	len = ALIGN_N_BYTE(len, 4);
	for (i = 0; i < (len / 4); i++) {
		*((unsigned int *)(dest)) = *((unsigned int *)(src));
		dest += 4;
		src += 4;
	}
	for (i = 0; i < (len % 4); i++) {
		*dest = *src;
		dest++;
		src++;
	}
	return 0;
}

int pcie_bar_write(int bar, int offset, unsigned char *buf, int len)
{
	m6x_pcie_t *priv = m6x_pcie_device;
	char *mem = priv->bar[bar].vmem;
	mem += offset;
	hwcopy(mem, buf, len);
	return 0;
}

int pcie_bar_read(int bar, int offset, unsigned char *buf, int len)
{
	int i;
	m6x_pcie_t *priv = m6x_pcie_device;
	unsigned char *mem = priv->bar[bar].vmem + offset;
	
	len = ALIGN_N_BYTE(len, 4);
	for(i=0; i<len/4; i++)
	{
		*(unsigned int *)(buf + i*4)  = *(unsigned int *)(mem + i*4);
	}
	return 0;
}

int m6x_reg_read(unsigned int reg, unsigned int *value)
{
	pcie_bar_read(0, (int)reg,  (unsigned char *)value, 4);
	printk(KERN_ERR "reg_read(0x%x, 0x%x)\n", reg, *value);
    return 0;
}

int m6x_reg_bit_clr(unsigned int reg, int bit)
{
	unsigned int value;
	
	pcie_bar_read(0, reg,  (unsigned char *)&value, 4);
	BIT_CLR(value, bit);
	pcie_bar_write(0, reg,  (unsigned char *)&value, 4);
	//printk(KERN_ERR "reg_bit_clr(0x%x, %d)\n", reg, bit);
	return 0;
}
int m6x_reg_bit_set(unsigned int reg, int bit)
{
	unsigned int value;
	
	pcie_bar_read(0, reg,  (unsigned char *)&value, 4);
	BIT_SET(value, bit);
	pcie_bar_write(0, reg,  (unsigned char *)&value, 4);
	//printk(KERN_ERR "reg_bit_set(0x%x, %d)\n", reg, bit);
	return 0;
}
int m6x_reg_write(unsigned int reg, unsigned int value)
{
	pcie_bar_write(0, reg, (unsigned char *)&value, 4);
	printk(KERN_ERR "reg_write(0x%x, 0x%x)\n", reg, value);
    return 0;
}


int hw_xmit(unsigned long phy, int len)
{
	unsigned int value;
	printk(KERN_ERR "%s(0x%lx, %d)\n", __func__, phy, len);

	m6x_reg_read(0x2c, &value);
	printk(KERN_ERR "reg[0x2c]:0x%x\n", value);
	if(value &(1<<8))
	{	
		m6x_reg_bit_clr(0x2c, 8);
		m6x_reg_bit_set(0x2c, 8);
	}
	m6x_reg_write(0x40, (unsigned int)phy);
	m6x_reg_write(0x44, (unsigned int)(phy>>32));
	m6x_reg_write(0x24, len);

	m6x_reg_bit_clr(0x28,8) ;
	m6x_reg_bit_set(0x28, 8);
	m6x_reg_bit_clr(0x28,8) ;
	return 0;
}


static unsigned int  *write_dma_buf_word;
static unsigned char *write_dma_buf_char;
static unsigned int  *read_dma_buf_word;
static unsigned char *read_dma_buf_char;


int tx_test(int value)
{
	int i;
	static int flag = 0;
	static dm_t dm;
	static unsigned int  *dword;
	static unsigned char *buf;
	if(flag == 0)
	{
		dmalloc(&dm, 512);
		dword = (unsigned int  *)(dm.vir);
		buf    = (unsigned char *)(dm.vir);
		write_dma_buf_word = dword ;
		write_dma_buf_char = buf ;
		
		flag = 1;
	}
#if 0
	for(i=0; i<(512/4); i++)
	{
		dword[i] = value++;
	}
	for(i=0; i<256; i++)
		buf[i] = i;
#endif
	memset(buf, 0x00, 512);

	for(i=0; i<512; i++)
			buf[i] = value;

	hexdump("tx_test", buf, 512);
	hw_xmit(dm.phy, 512);
	return 0;
	
}


int rx_test(int addr, int size)//0,4
{
	int i;
	static int flag = 0;
	static dm_t dm;
	static unsigned int  *dword;
	static unsigned char *buf;
	unsigned int value = 0 ;
	if(flag == 0)
	{
		dmalloc(&dm, 4096);
		dword  = (unsigned int  *)(dm.vir);
		buf    = (unsigned char *)(dm.vir);
		read_dma_buf_word = dword ;
		read_dma_buf_char = buf ;
		flag = 1;
	}
	memset(buf, 0x00, 4096);
	printk(KERN_ERR "phy(0xl%x, 0xl%x), virt(0x%lx, 0x%lx)\n", dm.phy, virt_to_phys(dm.vir), dm.vir, phys_to_virt(dm.phy) );
	m6x_reg_write(0x38, (unsigned int)(dm.phy) + addr);
	m6x_reg_write(0x3c, (unsigned int)(dm.phy>>32));
	m6x_reg_write(0xac, size);

	m6x_reg_bit_clr(0x28,8) ;
	m6x_reg_bit_clr(0x28,0) ;
	m6x_reg_bit_set(0x28, 0);// 动dma写    从网卡写数据到 内存中
	m6x_reg_bit_clr(0x28,0) ;

	m6x_reg_read( 0x28, &value) ;
	
	
	//hexdump("rx_test", buf+addr, size);

	return 0;
}

int read_dma_buf( int size){
	printk("==================================================\n") ;
	printk("==============Read Dma Start======================\n") ;
	printk("==================================================\n") ;
	hexdump("rx_test", read_dma_buf_char, size);
	printk("==================================================\n") ;
	printk("==============Read Dma End========================\n") ;
	printk("==================================================\n") ;
}

int m6x_pcie_msi_irq(int irq, void *arg)
{
	static int cnt = 0 , seq = 0;
	printk("---------------------------------------\n") ;
	printk("m6x_pcie_msi_irq  irq = %d\n", irq) ;
	printk("***************************************\n") ;

#if 0
	if (irq == (m6x_pcie_device->irq + 4)){
		printk("dma 写成功中断...\n") ;
		printk("start dma read \n") ;
		rx_test(0,512) ;
	}
#endif

	if (irq == (m6x_pcie_device->irq + 5)){
		printk("dma 接收中断\n") ;
		read_dma_buf(512) ;
		printk("dma read success...\n") ;
	}

	
#if 0
	if(irq == 33)
	{
		printk(KERN_ERR "msi(33)\n");
		/*
		if(m6x_buffer_len() > 0)
		{
			ioctl_trigger();	
		}
		*/
		m6x_reg_bit_clr(0x88, 0);
		m6x_reg_bit_set(0x88, 0);
		return IRQ_HANDLED;
	}
	if(irq == 30)
	{
		cnt++;
		seq++;
		if(cnt == 10)
		{
			printk(KERN_ERR "msi(30) count:%d\n",seq);
			cnt = 0;
		}
		return IRQ_HANDLED;
	}
#endif

	return IRQ_HANDLED;
}

static int m6x_pcie_probe(struct pci_dev *dev,
			   const struct pci_device_id *pci_id)
{

	m6x_pcie_t *priv = NULL;
	int ret = -ENODEV, i, flag;

	printk(KERN_ERR "%s Enter\n", __func__);
	priv = kzalloc(sizeof(m6x_pcie_t), GFP_KERNEL);
	if (!priv) {
		return -ENOMEM;
	}
	m6x_pcie_device = priv;
	priv->dev = dev;
	pci_set_drvdata(dev, priv);

	printk(" pci_enable_device before >>>\n") ;
	if (pci_enable_device(dev)) {/*使能PCI设备*/
		printk(KERN_ERR "%s cannot enable device\n", pci_name(dev));
		goto err_out;
	}
	printk(" pci_enable_device after >>>\n") ;
	
	pci_set_master(dev);/*设置为总线主ＤＭＡ*/
	priv->irq = dev->irq;
	printk(KERN_ERR "dev->irq %d\n", dev->irq);
	priv->legacy_en = 0;
	priv->msi_en = 1;
	if(priv->legacy_en == 1) {
		priv->irq = dev->irq;
	}
	else if(priv->msi_en == 1) {
		priv->irq_num = pci_msi_vec_count(dev);
		printk(KERN_ERR "pci_msix_vec_count ret %d\n", priv->irq_num);
		ret = pci_enable_msi_range(dev, 1, priv->irq_num);
		if (ret > 0) {
			printk(KERN_ERR "pci_enable_msi_range %d ok\n", ret);
			priv->msi_en = 1;
		} else {
			printk(KERN_ERR "pci_enable_msi_range err\n");
			priv->msi_en = 0;
		}
		priv->irq = dev->irq;
	}
	printk(KERN_ERR "dev->irq %d\n", dev->irq);
	printk(KERN_ERR "legacy %d msi_en %d\n",priv->legacy_en, priv->msi_en);
	for (i = 0; i < 8; i++) {
		flag = pci_resource_flags(dev, i);
		if (!(flag & IORESOURCE_MEM)) {
			continue;
		}
		priv->bar[i].mmio_start = pci_resource_start(dev, i);
		priv->bar[i].mmio_end = pci_resource_end(dev, i);
		priv->bar[i].mmio_flags = pci_resource_flags(dev, i);
		priv->bar[i].mmio_len = pci_resource_len(dev, i);
		priv->bar[i].mem =
		    ioremap(priv->bar[i].mmio_start, priv->bar[i].mmio_len);
		priv->bar[i].vmem = priv->bar[i].mem;
		if (priv->bar[i].vmem == NULL) {
			printk(KERN_ERR "%s:cannot remap mmio, aborting\n",
			       pci_name(dev));
			ret = -EIO;
			goto err_out;
		}
		printk(KERN_ERR "BAR(%d) (0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx)\n", i,
		       (unsigned long)priv->bar[i].mmio_start,
		       (unsigned long)priv->bar[i].mmio_end,
		       priv->bar[i].mmio_flags,
		       (unsigned long)priv->bar[i].mmio_len,
		       (unsigned long)priv->bar[i].vmem);
	}
	priv->bar_num = 8;
	ret = pci_request_regions(dev, DRVER_NAME);/*申请IO或内存资源*/
	if (ret) {
		priv->in_use = 1;
		goto err_out;
	}

#if 0
	if(priv->legacy_en == 1)
	{
		ret = request_irq(priv->irq,
				(irq_handler_t) (&m6x_pcie_legacy_irq),
				IRQF_SHARED, DRVER_NAME, (void *)priv);
		/*
		ret = request_irq(priv->irq,
				(irq_handler_t) (&m6x_pcie_legacy_irq),
				IRQF_NO_SUSPEND | IRQF_NO_THREAD | IRQF_PERCPU, 
				DRVER_NAME, (void *)priv);
		*/
		if (ret)
			printk(KERN_ERR "%s request_irq(%d), error %d\n", __func__, priv->irq, ret);
		else
			printk(KERN_ERR "%s request_irq(%d) ok\n", __func__, priv->irq);
	}
#endif

	if (priv->msi_en == 1) {
		for (i = 0; i < priv->irq_num; i++) {
			ret =
			    request_irq(priv->irq + i,
					(irq_handler_t) (&m6x_pcie_msi_irq),
					IRQF_SHARED, DRVER_NAME, (void *)priv);
			if (ret) {
				printk(KERN_ERR "%s request_irq(%d), error %d\n",
				       __func__, priv->irq + i, ret);
				break;
			}
			printk(KERN_ERR "%s request_irq(%d) ok\n", __func__, priv->irq + i);
		}
		if (i == priv->irq_num)
			priv->irq_en = 1;
		//pci_write_config_dword(dev, 0x5c, 0);
		//pci_write_config_dword(dev, 0x60, 0);		
	}
	device_wakeup_enable(&(dev->dev));

	printk(KERN_ERR "%s ok\n", __func__);
	return 0;

err_out:
	if (priv) {
		kfree(priv);
	}
	return ret;
}

static void m6x_pcie_remove(struct pci_dev *dev)
{
	int i;
	m6x_pcie_t *priv;
	priv = (m6x_pcie_t *) pci_get_drvdata(dev);
	if(priv->legacy_en == 1) {
		free_irq(priv->irq, (void *)priv);
	}
	if (1 == priv->msi_en) {
		for (i = 0; i < priv->irq_num; i++) {
			free_irq(priv->irq + i, (void *)priv);
		}
		pci_disable_msi(dev);
	}
	for (i = 0; i < priv->bar_num; i++) {
		if (NULL != priv->bar[i].mem)
			iounmap(priv->bar[i].mem);
	}
	pci_release_regions(dev);
	kfree(priv);
	pci_set_drvdata(dev, NULL);
	pci_disable_device(dev);
}



/**/
static struct pci_device_id m6x_pcie_tbl[] = {
	//{0x10ee, 0x0007, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0x10ee, 0x0088, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0,}
};


MODULE_DEVICE_TABLE(pci, m6x_pcie_tbl);
static struct pci_driver m6x_pcie_driver = {
	.name = DRVER_NAME,
	.id_table = m6x_pcie_tbl,
	.probe = m6x_pcie_probe,
	.remove = m6x_pcie_remove,
};
struct timer_list dma_send_timer;
struct timer_list read_reg_timer;


void read_reg_timer_func(unsigned long data)
{
	unsigned int value = 0;
	static int i = 1 ;
	/*往dma中对应地址写数据*/
	printk("read_reg_timer_func i = %d\n", i++) ;

#if 0
	m6x_reg_write(0x00, 1) ;
	m6x_reg_read( 0x00, &value) ;
	value = 0 ;
	m6x_reg_write(0xac, 2) ;
	m6x_reg_read( 0xac, &value) ;
	value = 0 ;
	m6x_reg_write(0x38, 3) ;
	m6x_reg_read( 0x38, &value) ;
	value = 0 ;
	m6x_reg_write(0x24, 4) ;
	m6x_reg_read( 0x24, &value) ;
#endif

	rx_test(0,512) ;

	
	
	if( i < 3)
		mod_timer(&(read_reg_timer), jiffies + msecs_to_jiffies(1000));
	return;
}

void dma_send_timer_func(unsigned long data)
{
	static int i = 1 ;
	/*往dma中对应地址写数据*/
	tx_test(i) ;
	printk("dma_send_timer_func i = %d\n", i++) ;
	
	if( i < 3)
		mod_timer(&(dma_send_timer), jiffies + msecs_to_jiffies(1000));
	return;
}

static int __init dma_test_init(void)
{
	int ret = 0;
	
	ret = pci_register_driver(&m6x_pcie_driver);
#if 1
	dma_send_timer.function = dma_send_timer_func;
	init_timer(&dma_send_timer);
	//mod_timer(&(dma_send_timer), jiffies + msecs_to_jiffies(1000));
#endif
	read_reg_timer.function = read_reg_timer_func;
	init_timer(&read_reg_timer);
	mod_timer(&(read_reg_timer), jiffies + msecs_to_jiffies(2000));


	
	return ret;
}

static void __exit dma_test_exit(void)
{
	printk(KERN_ERR "dma_test_exit exit\n");
	pci_unregister_driver(&m6x_pcie_driver);


}

module_init(dma_test_init);
module_exit(dma_test_exit);

MODULE_LICENSE("GPL");



