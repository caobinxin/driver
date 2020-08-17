/*
 * Copyright (C) 2016 http://www.qulsar.cn/
 *
 * Authors	: jinglong.chen@foxmail.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "m6x_api.h"

#define DRVER_NAME      "m6x"

int base_irq ;

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

int dmfree(dm_t *dm)
{
	struct device *dev = &(m6x_pcie_device->dev->dev);
	if (NULL == dev) {
		printk(KERN_ERR "%s(NULL)\n", __func__);
		return -1;
	}
	printk(KERN_ERR "dma_free_coherent(%d,0x%lx,0x%lx)\n", dm->size, dm->vir, dm->phy);
	dma_free_coherent(dev, dm->size, (void *)(dm->vir), dm->phy);
	memset(dm, 0x00, sizeof(dm_t));
	return -1;
}
int m6x_uart_rx_work(void);
int m6x_buffer_len(void);
int ioctl_trigger(void);
int m6x_pcie_msi_irq(int irq, void *arg)
{
	static int cnt = 0 , seq = 0;
	printk("create irq(%d) >>>\n", irq) ;

	if( irq == ( base_irq + 3 ) )
	{
		printk(KERN_ERR "handle msi(%d) ...\n", irq);
		m6x_uart_rx_work();
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
		m6x_pps_interrupt();
		return IRQ_HANDLED;
	}
	return IRQ_HANDLED;
}

int m6x_pcie_legacy_irq(int irq, void *arg)
{
	//unsigned int value = 0;
	unsigned long irq_flags;
	/*
	m6x_reg_read(0x300, &value);
	if(value & (1<<30)) {
		m6x_uart_interrupt();
	} else if(value & (1<<0)) {
		m6x_pps_interrupt();
	}
	*/
	local_irq_save(irq_flags);
	printk(KERN_ERR "pps\n");
	//m6x_reg_bit_set(0x00, 0);
	//m6x_reg_bit_clr(0x00, 0);
	local_irq_restore(irq_flags);
	return IRQ_HANDLED;
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
	//printk(KERN_ERR "reg_read(0x%x, 0x%x)\n", reg, *value);
    return 0;
}

int m6x_reg_write(unsigned int reg, unsigned int value)
{
	pcie_bar_write(0, reg, (unsigned char *)&value, 4);
	printk(KERN_ERR "reg_write(0x%x, 0x%x)\n", reg, value);
    return 0;
}

int m6x_mreg_write(unsigned int reg, unsigned char *buf, int len)
{
	m6x_pcie_t *priv = m6x_pcie_device;
	unsigned char *mem = (unsigned char *)(priv->bar[0].vmem);
	printk(KERN_ERR "%s(0x%x, 0x%x)\n", __func__, reg, len);
	
	mem += reg;
	hwcopy(mem, buf, len);
	return 0;
}

int m6x_regs_dump(void)
{
	int i;
	unsigned int regs[64];

	memset((char *)regs, 0x00, sizeof(regs));
	for(i=0; i<64; i++)
	{
		m6x_reg_read(i*4, &regs[i]);
	}
	hexdump("regs", (unsigned char *)regs, sizeof(regs));
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

int m6x_reg_bit_clr(unsigned int reg, int bit)
{
	unsigned int value;
	
	pcie_bar_read(0, reg,  (unsigned char *)&value, 4);
	BIT_CLR(value, bit);
	pcie_bar_write(0, reg,  (unsigned char *)&value, 4);
	//printk(KERN_ERR "reg_bit_clr(0x%x, %d)\n", reg, bit);
	return 0;
}

char *pcie_bar_vmem(int bar)
{
	m6x_pcie_t *priv = m6x_pcie_device;
	if (NULL == priv) {
		printk(KERN_ERR "m6x_pcie_device NULL\n");
		return NULL;
	}
	return priv->bar[bar].vmem;
}

int pcie_config_read(int offset, char *buf, int len)
{
	int i, ret;
	m6x_pcie_t *priv = m6x_pcie_device;

	for (i = 0; i < len; i++) {
		ret = pci_read_config_byte(priv->dev, i, &(buf[i]));
		if (0 != ret) {
			printk(KERN_ERR "pci_read_config_dword %d err\n", ret);
			return -1;
		}
	}
	return 0;
}

int pcie_config_write(int offset, char *buf, int len)
{
	int i, ret;
	m6x_pcie_t *priv = m6x_pcie_device;

	for (i = 0; i < len; i++) {
		ret = pci_write_config_byte(priv->dev, i, buf[i]);
		if (0 != ret) {
			printk(KERN_ERR "pcie_config_write %d err\n", ret);
			return -1;
		}

	}
	return 0;
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

	if (pci_enable_device(dev)) {/*使能PCI设备*/
		printk(KERN_ERR "%s cannot enable device\n", pci_name(dev));
		goto err_out;
	}
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
	base_irq = dev->irq ;
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
	if (priv->msi_en == 1) {
		printk("priv->msi_en == 1\n") ;
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
	m6x_uart_init();
	m6x_pps_init();
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

static int __init m6x_pcie_init(void)
{
	int ret = 0;
	
	printk(KERN_ERR "m6x_pcie_init init\n");
	m6x_ioctl_init();
	ret = pci_register_driver(&m6x_pcie_driver);
	printk(KERN_ERR "pci_register_driver ret %d\n", ret);
	return ret;
}

static void __exit m6x_pcie_exit(void)
{
	printk(KERN_ERR "m6x_pcie_exit exit\n");
	m6x_ioctl_exit();
	pci_unregister_driver(&m6x_pcie_driver);
}

module_init(m6x_pcie_init);
module_exit(m6x_pcie_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jinglong.chen");
MODULE_DESCRIPTION("marlin3 pcie/edma drv");
