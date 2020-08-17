#include "pcie_net_dri.h"

#define DRVER_NAME      "pcie_net_m6x"

char shortpkt[ETH_ZLEN] ;

static int debug = 3 ;

unsigned int pcie_debug = 0b11111111 ;

pcie_ether_ptp_m6x_pri_data_t *m6x_pri_data = NULL ;

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
	pcie_ether_ptp_m6x_pri_data_t *priv = m6x_pri_data;
	char *mem = priv->bar[bar].vmem;
	mem += offset;
	hwcopy(mem, buf, len);
	return 0;
}

int pcie_bar_read(int bar, int offset, unsigned char *buf, int len)
{
	int i;
	pcie_ether_ptp_m6x_pri_data_t *priv = m6x_pri_data;
	unsigned char *mem = priv->bar[bar].vmem + offset;
	
	len = ALIGN_N_BYTE(len, 4);
	for(i=0; i<len/4; i++)
	{
		*(unsigned int *)(buf + i*4)  = *(unsigned int *)(mem + i*4);
	}
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

int receive_dma_ops(struct net_device *netdev)
{
	int i;
	static int flag = 0;
    dm_t *dm = NULL ;
	static unsigned int  *dword;
	static unsigned char *buf;
	static int size = 4096 ;
	pcie_ether_ptp_m6x_pri_data_t *priv = netdev_priv( netdev) ;
	dm = &priv->rx_dm ;
	PDEBUG( DEBUG7, "接收缓冲区的地址信息 藏在 priv->rx_dm 中") ;
	if(flag == 0)
	{
		dmalloc(netdev,dm, size);
		dword  = (unsigned int  *)(dm->vir);
		buf    = (unsigned char *)(dm->vir);
		flag = 1;
	}
	memset(buf, 0x00, 4096);
	printk(KERN_ERR "phy(0xl%x, 0xl%x), virt(0x%lx, 0x%lx)\n", dm->phy, virt_to_phys(dm->vir), dm->vir, phys_to_virt(dm->phy) );
	PDEBUG( DEBUG7, "接收缓冲区的大小是 %d\n", size) ;
	PDEBUG( DEBUG7, "将接收 缓冲区的物理地址 缓冲区的大小 告诉网卡中的寄存器  0x38 , 0x3c, 0xac, 0x28\n") ;
	m6x_reg_write(0x38, (unsigned int)(dm->phy));
	m6x_reg_write(0x3c, (unsigned int)(dm->phy>>32));
	m6x_reg_write(0xac, size);
	
	//hexdump("rx_test", buf, size);
	
	return 0;
}

int send_dma_ops(struct net_device *netdev)
{
	int i;
	static int flag = 0;
    dm_t *dm = NULL ;
	static unsigned int  *dword;
	static unsigned char *buf;
	static int size = 4096 ;
	pcie_ether_ptp_m6x_pri_data_t *priv = netdev_priv( netdev) ;
	dm = &priv->tx_dm ;
	PDEBUG( DEBUG7, "发送缓冲区的地址信息 藏在 priv->tx_dm 中") ;
	if(flag == 0)
	{
		dmalloc(netdev,dm, size);
		dword  = (unsigned int  *)(dm->vir);
		buf    = (unsigned char *)(dm->vir);
		flag = 1;
	}
	memset(buf, 0x00, 4096);
	printk(KERN_ERR "phy(0xl%x, 0xl%x), virt(0x%lx, 0x%lx)\n", dm->phy, virt_to_phys(dm->vir), dm->vir, phys_to_virt(dm->phy) );
	PDEBUG( DEBUG7, "发送缓冲区的大小是 %d\n", size) ;
	return 0;
}


int dmalloc(struct net_device *netdev,dm_t *dm, int size)
{
	pcie_ether_ptp_m6x_pri_data_t *priv = netdev_priv( netdev) ;
	struct device *dev = &(priv->pdev->dev);
	
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

int dmfree( struct net_device *netdev,dm_t *dm)
{
	pcie_ether_ptp_m6x_pri_data_t *priv = netdev_priv( netdev) ;
	struct device *dev = &(priv->pdev->dev);
	
	if (NULL == dev) {
		printk(KERN_ERR "%s(NULL)\n", __func__);
		return -1;
	}
	printk(KERN_ERR "dma_free_coherent(%d,0x%lx,0x%lx)\n", dm->size, dm->vir, dm->phy);
	dma_free_coherent(dev, dm->size, (void *)(dm->vir), dm->phy);
	memset(dm, 0x00, sizeof(dm_t));
	return -1;
}

static irqreturn_t m6x_pcie_msi_receive_start_irq( int irq, void *args)
{
	//手动打开DMA 传输
	PDEBUG( DEBUG7, "假接收中断....\n") ;
	
	PDEBUG( DEBUG7, "从特定寄存器中读出,需要接收的长度\n") ;
	PDEBUG( DEBUG7, "//TODO\n") ;
	
	PDEBUG( DEBUG7, "启动DMA接收传输\n") ;
	m6x_reg_bit_clr(0x28,8) ;
	m6x_reg_bit_clr(0x28,0) ;
	m6x_reg_bit_set(0x28, 0);// 动dma写    从网卡写数据到 内存中
	m6x_reg_bit_clr(0x28,0) ;
	
	return IRQ_HANDLED ;
}

int read_dma_buf(unsigned char *buf , int size){
	printk("==================================================\n") ;
	printk("==============Read Dma Start======================\n") ;
	printk("==================================================\n") ;
	hexdump("dma接收到的数据:", buf, size);
	printk("==================================================\n") ;
	printk("==============Read Dma End========================\n") ;
	printk("==================================================\n") ;
}


static irqreturn_t m6x_pcie_msi_receive_irq( int irq, void *args)
{
	int ret = 0 ;
	struct sk_buff *skb = NULL ;
	int size = 0 ;/*从寄存器中读出 接受到的数据的值*/

	pcie_ether_ptp_m6x_pri_data_t *priv = ( pcie_ether_ptp_m6x_pri_data_t *)args ;
	PDEBUG( DEBUG7, "真的接收中断...\n") ;
	PDEBUG(DEBUG7, "receive one package\n") ;
	

	/*2. 创建出skb*/
	skb = dev_alloc_skb (RX_LEN) ;
	skb->protocol = eth_type_trans(skb, priv->netdev) ;/*以太网协议*/

	/*1. 从dma中 读出数据 放到 skb->data中*/

	/*1.0 从寄存器中获取 size的长度 */
	size = 1536 ;
	PDEBUG( DEBUG7, "需要接收的数据长度为:%d\n", size) ;

	memcmp( skb->data, priv->rx_dm.vir, size) ;

	read_dma_buf( (unsigned char *) priv->rx_dm.vir, size) ;

	/*3. 将skb提交到上层协议层*/
	netif_rx( skb) ;
	PDEBUG( DEBUG7, "已将 接收skb 提交到协议栈中") ;
	
	return IRQ_HANDLED ;
}

static irqreturn_t m6x_pcie_msi_send_comple_irq( int irq, void *args)
{
	int ret = 0 ;
	pcie_ether_ptp_m6x_pri_data_t *priv = ( pcie_ether_ptp_m6x_pri_data_t *)args ;

	PDEBUG(DEBUG7, "send package successful\n") ;
	/*1.释放 发送时的SKB */
	PDEBUG(DEBUG7, "释放skb") ;
	dev_kfree_skb( priv->skb) ;

	PDEBUG( DEBUG7, "启动协议栈发送") ;
	netif_wake_queue( priv->netdev) ;/*等每次 发送完,在启动协议栈发送下一次 帧*/

	return IRQ_HANDLED ;
}

static int m6x_open(struct net_device *netdev)
{
	int ret = 0 ;
	pcie_ether_ptp_m6x_pri_data_t *priv = NULL ;
	priv = netdev_priv(netdev) ;

	/*1. request_region(), request_irq(), 等系统资源的申请*/

	/*1.0 网卡数据到来，需 启动DMA 传输  中断向量号为1*/

	ret = request_irq(priv->irq + INTRUPT_RECEIVE_START,(irq_handler_t) (&m6x_pcie_msi_receive_start_irq),IRQF_SHARED, DRVER_NAME, (void *)priv);
	if (ret) {
		if( ret == -EBUSY) printk(KERN_ERR"Device or resource busy\n") ;
		if( ret == -EINVAL) printk(KERN_ERR"Invalid argument\n") ;
		printk(KERN_ERR "%s request_irq(%d) : receive_irq, error %d\n",__func__, priv->irq + INTRUPT_RECEIVE_START, ret);
		return -1 ;
	}
	printk(KERN_ERR "%s request_irq(%d) receive_irq ok\n", __func__, priv->irq + INTRUPT_RECEIVE_START);

#if 1	
	/*1.1 DMA 将数据 搬移到 内存中产生 中断向量号为5*/

	ret = request_irq(priv->irq + INTRUPT_RECEIVE_COMPLE,(irq_handler_t) (&m6x_pcie_msi_receive_irq),IRQF_SHARED, DRVER_NAME, (void *)priv);
	if (ret) {
		if( ret == -EBUSY) printk(KERN_ERR"Device or resource busy\n") ;
		if( ret == -EINVAL) printk(KERN_ERR"Invalid argument\n") ;
		printk(KERN_ERR "%s request_irq(%d) : receive_irq, error %d\n",__func__, priv->irq + INTRUPT_RECEIVE_COMPLE, ret);
		goto ERROR_0 ;
	}
	printk(KERN_ERR "%s request_irq(%d) receive_irq ok\n", __func__, priv->irq + INTRUPT_RECEIVE_COMPLE);

	//pci_write_config_dword(dev, 0x5c, 0);
	//pci_write_config_dword(dev, 0x60, 0);		
	/*1.2 网口数据发送完成中断*/
	ret = request_irq(priv->irq + INTRUPT_SEND_COMPLE,(irq_handler_t) (&m6x_pcie_msi_send_comple_irq),IRQF_SHARED, DRVER_NAME, (void *)priv);
	if (ret) {
		if( ret == -EBUSY) printk(KERN_ERR"Device or resource busy\n") ;
		if( ret == -EINVAL) printk(KERN_ERR"Invalid argument\n") ;
		printk(KERN_ERR "%s request_irq(%d) : send_comple_irq, error %d\n",__func__, priv->irq + INTRUPT_SEND_COMPLE, ret);
		goto ERROR_1 ;
	}
	printk(KERN_ERR "%s request_irq(%d)  send_comple_irq ok\n", __func__, priv->irq + INTRUPT_SEND_COMPLE);
#endif	
	priv->irq_ready = true ;

	/*2. 访问硬件拿到 mac地址，赋值给 dev->dev_addr*/
	memcpy(netdev->dev_addr, "\0RAMLx", 7) ;
	printk(KERN_INFO"MACADDR=%s\n",netdev->dev_addr) ;

	/*3. 硬件相关操作*/
	/*3.1 激活内部PHY*/
	/*3.2 复位并激活网卡*/

	/*4. 开启接口传输队列 netif_start_queue(),*/
	netif_start_queue(netdev) ;
	
	return 0 ;

ERROR_1:
	free_irq(priv->irq + INTRUPT_RECEIVE_COMPLE, (void *)priv);

ERROR_0:
	free_irq(priv->irq + INTRUPT_RECEIVE_START, (void *)priv);

	priv->irq_ready = false ;
	
	return -1 ;

}

static int m6x_close(struct net_device *netdev)
{
	int ret = 0 ;
	pcie_ether_ptp_m6x_pri_data_t *priv = NULL ;
	priv = netdev_priv(netdev) ;

	/*1. 释放open 阶段申请的系统资源*/

	//1.1 释放申请的中断
	if(priv->irq_ready)
	{
		free_irq(priv->irq + INTRUPT_RECEIVE_START, (void *)priv);
#if 1
		free_irq(priv->irq + INTRUPT_RECEIVE_COMPLE, (void *)priv);
		free_irq(priv->irq + INTRUPT_SEND_COMPLE, (void *)priv);
#endif
		priv->irq_ready = false ;
	}


	pci_disable_msi(priv->pdev); /* 每次释放 完中断后，记得将 msi 清除*/

	/*2. 停止接口传输队列 netif_stop_queue(),*/
	netif_stop_queue( netdev) ;
	return ret ;
}

int m6x_hw_tx(char *data, int len, struct net_device *netdev)
{
	int ret = 0 ;
	pcie_ether_ptp_m6x_pri_data_t *priv = netdev_priv( netdev) ;

	//priv->send_dma_phy_addr = dma_map_single( &priv->pdev->dev, data, len, PCI_DMA_TODEVICE) ; /*这里拿到的是物理地址*/

	/*启动硬件开始dma发送*/
	PDEBUG(DEBUG7, "DMA 发送\n") ;
	hw_xmit( priv->tx_dm.phy, len) ;
	PDEBUG(DEBUG7, "DMA 发送结束\n") ;


	return ret ;
}

int m6x_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	int ret = 0 ;

	int len ;
	char *data ;
	 pcie_ether_ptp_m6x_pri_data_t *priv = netdev_priv(netdev) ;

	/*1. 对要发送数据的修改*/
	data = skb->data ;
	len = skb->len ;
	PDEBUG( DEBUG7, "发送长度 len = %d\n", len) ;
	if ( len < ETH_ZLEN)
	{
		PDEBUG( DEBUG7, "发送包数据过小... 换小包发送\n") ;
		memset(shortpkt, 0, ETH_ZLEN) ;
		memcpy(shortpkt, skb->data, skb->len) ;
		len = ETH_ZLEN ;
		data = shortpkt ;
	}
	netdev->trans_start = jiffies ;/*保存时间戳*/

	/*2. 记住skb，可以在中断时刻 释放*/
	priv->skb = skb ;
	
	memcpy(priv->tx_dm.vir, skb->data, skb->len) ;
	/*3. 由实际的硬件去发送*/
	hexdump("send data ->:", data, len) ;
	m6x_hw_tx(data, len, netdev) ;

	/*4. 协议栈停止回调 m6x_start_xmit 函数*/
	netif_stop_queue( netdev) ;
	PDEBUG(DEBUG7, "协议栈停止回调 m6x_start_xmit 停止发送skb") ;

	return ret ;
}

static void m6x_tx_timeout(struct net_device *netdev)
{
	pcie_ether_ptp_m6x_pri_data_t *priv = netdev_priv( netdev) ;
	PDEBUG( DEBUG0 ,"Transmit timeout at %ld, latency %ld \n", jiffies, 
													  jiffies - netdev->trans_start) ;
	priv->stats.tx_errors++ ;
	netif_wake_queue(netdev) ;
	return ;
}
#if 1
static const struct net_device_ops m6x_netdev_ops = {
	.ndo_open		= m6x_open,
	.ndo_stop		= m6x_close,
	.ndo_start_xmit		= m6x_start_xmit,
	.ndo_tx_timeout		= m6x_tx_timeout,
#if 0
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_rx_mode	= m6x_set_multicast_list,
	.ndo_set_mac_address	= m6x_set_mac_address,
	.ndo_change_mtu		= m6x_change_mtu,
	.ndo_do_ioctl		= m6x_do_ioctl,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= m6x_netpoll,
#endif
	.ndo_set_features	= m6x_set_features,
#endif
};
#endif




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

int m6x_pcie_msi_irq(int irq, void *arg)
{
	static int cnt = 0 , seq = 0;
	
	if(irq == 33)
	{
		printk(KERN_ERR "msi(33)\n");

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
	//	m6x_pps_interrupt();
		return IRQ_HANDLED;
	}
	return IRQ_HANDLED;
}
static int m6x_pcie_dev_init_ops(pcie_ether_ptp_m6x_pri_data_t * m6x_net_pri_data_p)
{
	
	int ret = -ENODEV, i, flag;
	pcie_ether_ptp_m6x_pri_data_t *priv = NULL ;

	if ( NULL == m6x_net_pri_data_p)
	{
		printk("[%s][%s][%d] input is error . \n", __FILE__, __func__, __LINE__) ;
		return -1 ;
	}

	priv = m6x_net_pri_data_p ;

	printk(KERN_ERR "%s Enter\n", __func__);
	
	if (pci_enable_device(priv->pdev)) {
		printk(KERN_ERR "%s cannot enable device\n", pci_name(priv->pdev));
		goto err_out;
	}
	pci_set_master(priv->pdev);
	priv->irq = priv->pdev->irq;
	printk(KERN_ERR "priv->pdev->irq %d\n", priv->pdev->irq);
	priv->legacy_en = 0;
	priv->msi_en = 1;
	if(priv->legacy_en == 1) {
		priv->irq = priv->pdev->irq;
	}
	else if(priv->msi_en == 1) {
		priv->irq_num = pci_msi_vec_count(priv->pdev);/*获取设备申请的中断向量个数*/
		printk(KERN_ERR "pci_msix_vec_count ret %d  获取设备申请的中断向量个数\n", priv->irq_num);
		ret = pci_enable_msi_range(priv->pdev, 1, priv->irq_num);
		if (ret > 0) {
			printk(KERN_ERR "pci_enable_msi_range %d ok 允许驱动申请 1 到 %d 个msi中断\n", ret, ret);
			priv->msi_en = 1;
		} else {
			printk(KERN_ERR "pci_enable_msi_range err\n");
			priv->msi_en = 0;
		}
		priv->irq = priv->pdev->irq;
	}
	printk(KERN_ERR "priv->pdev->irq %d\n", priv->pdev->irq);
	printk(KERN_ERR "legacy %d msi_en %d\n",priv->legacy_en, priv->msi_en);
	for (i = 0; i < 8; i++) {
		flag = pci_resource_flags(priv->pdev, i);
		if (!(flag & IORESOURCE_MEM)) {/*x86使用的是 IO 端口，所以这里将 IO 内存（ARM）相关的全部过滤掉*/
			continue;
		}
		priv->bar[i].mmio_start = pci_resource_start(priv->pdev, i);
		priv->bar[i].mmio_end = pci_resource_end(priv->pdev, i);
		priv->bar[i].mmio_flags = pci_resource_flags(priv->pdev, i);
		priv->bar[i].mmio_len = pci_resource_len(priv->pdev, i);
		priv->bar[i].mem =
		    ioremap(priv->bar[i].mmio_start, priv->bar[i].mmio_len);
		priv->bar[i].vmem = priv->bar[i].mem;
		if (priv->bar[i].vmem == NULL) {
			printk(KERN_ERR "%s:cannot remap mmio, aborting\n",
			       pci_name(priv->pdev));
			ret = -EIO;
			goto err_out;
		}
		printk(KERN_ERR "BAR(%d) (\n        mmio_start = 0x%lx,\n        mmio_end = 0x%lx,\n        mmio_flags = 0x%lx,\n        mmio_len = 0x%lx,\n        vmem = 0x%lx\n)\n", i,
		       (unsigned long)priv->bar[i].mmio_start,
		       (unsigned long)priv->bar[i].mmio_end,
		       priv->bar[i].mmio_flags,
		       (unsigned long)priv->bar[i].mmio_len,
		       (unsigned long)priv->bar[i].vmem);
	}
	priv->bar_num = 8;
	ret = pci_request_regions(priv->pdev, DRVER_NAME);/*通知内核 该设备 对应的IO端口 和 内存 资源已经使用， 其他的pci设备不要使用这个区域*/
	if (ret) {
		priv->in_use = 1;
		goto err_out;
	}
	device_wakeup_enable(&(priv->pdev->dev));
	printk(KERN_ERR "%s ok\n", __func__);
	return 0;

err_out:
	return ret;
}
static void m6x_free(pcie_ether_ptp_m6x_pri_data_t* priv)
{
	if (priv->mem) {
		pci_free_consistent(priv->pdev, sizeof(struct mem),
			priv->mem, priv->dma_addr);
		priv->mem = NULL;
	}
}
static int pcie_net_probe(struct pci_dev *pdev,
			   const struct pci_device_id *pci_id)
{
	int err ;
	struct net_device *netdev = NULL ;
	int ret = 0 ;
	pcie_ether_ptp_m6x_pri_data_t *m6x_net_pri_data_p = NULL ;/*m6x 网络设备的私有数据*/

	if ( !(netdev = alloc_etherdev(sizeof(pcie_ether_ptp_m6x_pri_data_t)))) /* 分配网络设备结构体*/
	{
		printk("[%s][%s][%d] alloc_etherdev fail !!! \n",__FILE__, __func__, __LINE__) ;
		return -1 ;
	}

	/* net_dev init*/
	netdev->hw_features |= NETIF_F_RXFCS;
	netdev->priv_flags |= IFF_SUPP_NOFCS;
	netdev->hw_features |= NETIF_F_RXALL;
	netdev->watchdog_timeo = M6X_WATCHDOG_PERIOD;
	strncpy(netdev->name, pci_name(pdev), sizeof(netdev->name) - 1);
	m6x_net_pri_data_p = netdev_priv(netdev);/* 私用数据是藏在 netdev 结构体的后面*/
	m6x_pri_data = m6x_net_pri_data_p ;/*全局保存一份私有数据*/
	netdev->netdev_ops = &m6x_netdev_ops;

#if 0
	SET_ETHTOOL_OPS(netdev, &m6x_ethtool_ops);
	m6x_net_pri_data_p->mdio_ctrl = mdio_ctrl_hw;
#endif

#ifdef NAPI 
	netif_napi_add(netdev, &nic->napi, m6x_poll, E100_NAPI_WEIGHT);
#endif

	m6x_net_pri_data_p->netdev = netdev;
	m6x_net_pri_data_p->pdev = pdev;
	m6x_net_pri_data_p->msg_enable = (1 << debug) - 1;
	pci_set_drvdata(pdev, netdev);/*将网络设备结构体 藏在 pci_dev 的私用数据中*/

	printk("pci dev ops ... \n") ;
	/*pcie设备 相关操作*/
	m6x_pcie_dev_init_ops( m6x_net_pri_data_p) ;

	spin_lock_init( &m6x_net_pri_data_p->lock ) ;/*初始化 自旋锁*/
	mutex_init( &m6x_net_pri_data_p->addr_lock ) ;/*初始化 互斥锁*/

	//INIT_DELAYED_WORK( &m6x_net_pri_data_p->phy_poll, m6x_poll_work) ;
	
	/*映射接收 dma 空间*/
	PDEBUG( DEBUG7, "分配接收缓冲区地址和大小 全局就分配这一次,在固定的地址处接收网卡传回的数据") ;
	receive_dma_ops(netdev) ;
	PDEBUG( DEBUG7, "分配发送缓冲区地址和大小 全局就分配这一次,在固定的地址处发送网络协议栈的数据") ;
	send_dma_ops(netdev) ;

	strcpy(netdev->name, "eth_M6x(%d)");
	if ((err = register_netdev(netdev))) {
		netif_err(m6x_net_pri_data_p, probe, m6x_net_pri_data_p->netdev, "Cannot register net device, aborting\n");
		return err ;
	}
	 

	printk("%s Enter\n", __func__);
	return ret;
}

static void pcie_net_remove(struct pci_dev *dev)
{
	int i;
	int ret ;
	pcie_ether_ptp_m6x_pri_data_t *priv = NULL;
	struct net_device *netdev = pci_get_drvdata(dev);

	if ( netdev){

		priv = netdev_priv(netdev) ;

		for (i = 0; i < priv->bar_num; i++) {
			if (NULL != priv->bar[i].mem)
				iounmap(priv->bar[i].mem);
		}

		dmfree( priv->netdev, &priv->rx_dm) ;
		
		unregister_netdev( netdev) ;
		
		m6x_free(priv) ;
		
		free_netdev( netdev) ;

		
		pci_release_regions(dev) ;

		pci_set_drvdata(dev, NULL) ;
		
		pci_disable_device(dev) ;

	}
}
static struct pci_device_id pcie_net_tbl[] = {
	{0x10ee, 0x0088, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0,}
};

MODULE_DEVICE_TABLE(pci, pcie_net_tbl);
static struct pci_driver pcie_net_driver = {
	.name = DRVER_NAME,
	.id_table = pcie_net_tbl,
	.probe = pcie_net_probe,
	.remove = pcie_net_remove,
};

static int __init pcie_net_init(void)
{
	int ret = 0;
	printk("pcie_net_init init\n");
	ret = pci_register_driver(&pcie_net_driver);
	printk("pci_register_driver ret %d\n", ret);
	return ret;
}

static void __exit pcie_net_exit(void)
{
	printk("pcie_net_exit exit\n");
	pci_unregister_driver(&pcie_net_driver);
}

module_init(pcie_net_init);
module_exit(pcie_net_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("colby");
MODULE_DESCRIPTION("pcie network drv");
