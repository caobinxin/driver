/*
 * Copyright (C) 2016 xxxxx
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

#ifndef __PCIE_NET_DRI_H_
#define __PCIE_NET_DRI_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/signal.h>
#include <linux/timer.h>
#include <linux/vmalloc.h>
#include <linux/time.h>
#include <linux/dma-mapping.h>
#include <linux/pci_regs.h>
#include <linux/pci.h>
#include <linux/msi.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/pm_wakeup.h>
#include <linux/time.h>
#include <linux/kfifo.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#include <linux/hardirq.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/if_vlan.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/string.h>
#include <linux/firmware.h>
#include <linux/rtnetlink.h>
#include <asm/unaligned.h>

#define M6X_WATCHDOG_PERIOD ( 2 * HZ )

typedef struct
{
	unsigned long vir;
	unsigned long phy;
	int size;
}dm_t;

#define BIT_SET(r, v) ( r = ( r | 1<<v ))
#define BIT_CLR(r, v) ( r = ( r & (~(1<<v)) ) )
#define ALIGN_N_BYTE(p, n) (((unsigned int)p+n-1)&~(n-1))

struct rfd {
	__le16 status;
	__le16 command;
	__le32 link;
	__le32 rbd;
	__le16 actual_size;
	__le16 size;
};

enum ru_state  {
	RU_SUSPENDED = 0,
	RU_RUNNING	 = 1,
	RU_UNINITIALIZED = -1,
};

enum scb_cmd_lo {
	cuc_nop        = 0x00,
	ruc_start      = 0x01,
	ruc_load_base  = 0x06,
	cuc_start      = 0x10,
	cuc_resume     = 0x20,
	cuc_dump_addr  = 0x40,
	cuc_dump_stats = 0x50,
	cuc_load_base  = 0x60,
	cuc_dump_reset = 0x70,
};

enum mac {
	mac_82557_D100_A  = 0,
	mac_82557_D100_B  = 1,
	mac_82557_D100_C  = 2,
	mac_82558_D101_A4 = 4,
	mac_82558_D101_B0 = 5,
	mac_82559_D101M   = 8,
	mac_82559_D101S   = 9,
	mac_82550_D102    = 12,
	mac_82550_D102_C  = 13,
	mac_82551_E       = 14,
	mac_82551_F       = 15,
	mac_82551_10      = 16,
	mac_unknown       = 0xFF,
};

enum phy {
	phy_100a     = 0x000003E0,
	phy_100c     = 0x035002A8,
	phy_82555_tx = 0x015002A8,
	phy_nsc_tx   = 0x5C002000,
	phy_82562_et = 0x033002A8,
	phy_82562_em = 0x032002A8,
	phy_82562_ek = 0x031002A8,
	phy_82562_eh = 0x017002A8,
	phy_82552_v  = 0xd061004d,
	phy_unknown  = 0xFFFFFFFF,
};

struct param_range {
	u32 min;
	u32 max;
	u32 count;
};

struct params {
	struct param_range rfds;
	struct param_range cbs;
};

enum loopback {
	lb_none = 0, lb_mac = 1, lb_phy = 3,
};

typedef struct{
	resource_size_t mmio_start ;
	resource_size_t mmio_end ;
	resource_size_t mmio_len ;

	unsigned long mmio_flags ;
	unsigned char *mem ;
	unsigned char *vmem ;
} bar_info_t ;

typedef struct pcie_ether_data_t
{
	/* Begin: frequently used values: keep adjacent for cache effect */
	u32 msg_enable				____cacheline_aligned;
	struct net_device *netdev;
	struct pci_dev *pdev;
	u16 (*mdio_ctrl)( struct pcie_ether_data_t *m6x_net_pri_data_p, u32 addr, u32 dir, u32 reg, u16 data);

	struct rx *rxs				____cacheline_aligned;
	struct rx *rx_to_use;
	struct rx *rx_to_clean;
	struct rfd blank_rfd;
	enum ru_state ru_running;

	spinlock_t cb_lock			____cacheline_aligned;
	spinlock_t cmd_lock;
	struct csr __iomem *csr;
	enum scb_cmd_lo cuc_cmd;
	unsigned int cbs_avail;
	struct napi_struct napi;
	struct cb *cbs;
	struct cb *cb_to_use;
	struct cb *cb_to_send;
	struct cb *cb_to_clean;
	__le16 tx_command;
	/* End: frequently used values: keep adjacent for cache effect */

	enum {
		ich                = (1 << 0),
		promiscuous        = (1 << 1),
		multicast_all      = (1 << 2),
		wol_magic          = (1 << 3),
		ich_10h_workaround = (1 << 4),
	} flags					____cacheline_aligned;

	enum mac mac;
	enum phy phy;
	struct params params;
	struct timer_list watchdog;
	struct mii_if_info mii;
	struct work_struct tx_timeout_task;
	enum loopback loopback;

	struct mem *mem;
	dma_addr_t dma_addr;

	struct pci_pool *cbs_pool;
	dma_addr_t cbs_dma_addr;
	u8 adaptive_ifs;
	u8 tx_threshold;
	u32 tx_frames;
	u32 tx_collisions;
	u32 tx_deferred;
	u32 tx_single_collisions;
	u32 tx_multiple_collisions;
	u32 tx_fc_pause;
	u32 tx_tco_frames;

	u32 rx_fc_pause;
	u32 rx_fc_unsupported;
	u32 rx_tco_frames;
	u32 rx_short_frame_errors;
	u32 rx_over_length_errors;

	u16 eeprom_wc;
	__le16 eeprom[256];
	spinlock_t mdio_lock;
	const struct firmware *fw;

	/*pci_dev_pri_data*/
	struct pci_saved_state *save ;
	int legacy_en ;
	int msi_en ;
	int msix_en ;
	int in_use ;
	int irq ;
	int irq_num ;
	int irq_en ;
	int bar_num ;
	bar_info_t bar[8] ;
	struct msix_entry msix[100] ;

	/* board info */
	unsigned char revision ;
	unsigned char irq_pin ;
	unsigned char sub_vendor_id ;
	unsigned char sub_system_id ;
	unsigned char vendor_id ;
	unsigned char device_id ;

	struct sk_buff *skb ;
	struct stats {
		int tx_errors ;
	} stats ;

	struct mutex addr_lock ; /* 控制phy 和 eeprom 访问的锁*/
	struct delayed_work phy_poll ;

	spinlock_t lock ;
	dma_addr_t send_dma_phy_addr ;
	dm_t rx_dm ;
	dm_t tx_dm ;
	bool irq_ready ;

} pcie_ether_ptp_m6x_pri_data_t ;
/*
struct stats {
	__le32 tx_good_frames, tx_max_collisions, tx_late_collisions,
		tx_underruns, tx_lost_crs, tx_deferred, tx_single_collisions,
		t x_multiple_collisions, tx_total_collisions;
	__le32 rx_good_frames, rx_crc_errors, rx_alignment_errors,
		rx_resource_errors, rx_overrun_errors, rx_cdt_errors,
		rx_short_frame_errors;
	__le32 fc_xmt_pause, fc_rcv_pause, fc_rcv_unsupported;
	__le16 xmt_tco_frames, rcv_tco_frames;
	__le32 complete;
};
*/
struct mem {
	struct {
		u32 signature;
		u32 result;
	} selftest;
	/*struct stats stats;*/
	u8 dump_buf[596];
};
#define DEBUG 0b00000001

int pcie_bar_write(int bar, int offset, unsigned char *buf, int len);
int pcie_bar_read(int bar, int offset, unsigned char *buf, int len);
int m6x_reg_read(unsigned int reg, unsigned int *value);
int m6x_reg_write(unsigned int reg, unsigned int value);
int m6x_mreg_write(unsigned int reg, unsigned char *buf, int len);
int m6x_reg_bit_set(unsigned int reg, int bit);
int m6x_reg_bit_clr(unsigned int reg, int bit);

int m6x_uart_init(void);
int m6x_uart_rx(unsigned char *buf, int *len);
int m6x_uart_tx(unsigned char *buf, int len, int timeout);
int m6x_uart_rx_work(void);
int m6x_uart_reset(void);

int m6x_tod_display_set(int value);
int m6x_pps_tod_get(int id, unsigned char *report, int *len);
int m6x_pps_interrupt(void);
int m6x_pps_init(void);

int m6x_ioctl_init(void);
void m6x_ioctl_exit(void);
int hwcopy(unsigned char *dest, unsigned char *src, int len);
int hexdump(char *name, char *buf, int len);
int hex2char(unsigned int *arry, int len);

int m6x_buffer_init(void);
int m6x_buffer_put(unsigned char *buf, int len);
int m6x_buffer_get(unsigned char *buf, int *len);
int m6x_buffer_len(void);

int dmalloc(struct net_device *netdev, dm_t *dm, int size);
int dmfree(struct net_device *netdev, dm_t *dm);
int hw_xmit(unsigned long phy, int len);

int pcie_bar_read(int bar, int offset, unsigned char *buf, int len);

int m6x_reg_read(unsigned int reg, unsigned int *value);
int hw_xmit(unsigned long phy, int len);
int m6x_reg_bit_clr(unsigned int reg, int bit);
#define PCIE_NET_DRI_DEBUG

#define DEBUG0 0b00000001
#define DEBUG1 0b00000010
#define DEBUG2 0b00000100
#define DEBUG3 0b00001000
#define DEBUG4 0b00010000
#define DEBUG5 0b00100000
#define DEBUG6 0b01000000
#define DEBUG7 0b10000000

#ifdef PCIE_NET_DRI_DEBUG
	#define PDEBUG( level, fmt, args...) {\
				if ( pcie_debug & (level))\
					printk(KERN_INFO KBUILD_MODNAME ":[%s:%d] " fmt, \
							__func__, __LINE__, ##args);\
	}
#else
	#define PDEBUG(level, fmt, args...) do {} while(0) ;
#endif

#define INTRUPT_RECEIVE_START 1 //网口数据 到来 需要在这里启动 DMA 传输   然后等待 INTRUPT_RECEIVE_COMPLE 中断 到达
#define INTRUPT_SEND_COMPLE 4//FPGA 将网口数据 发送完成后，上报
#define INTRUPT_RECEIVE_COMPLE 5//DMA 将数据 搬移到 内存中，产生中断
#define RX_LEN 1500
#endif
