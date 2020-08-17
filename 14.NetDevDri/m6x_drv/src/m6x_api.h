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

#ifndef __M6X_API_H__
#define __M6X_API_H__

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

typedef struct
{
	unsigned long vir;
	unsigned long phy;
	int size;
}dm_t;

#define BIT_SET(r, v) ( r = ( r | 1<<v ))
#define BIT_CLR(r, v) ( r = ( r & (~(1<<v)) ) )
#define ALIGN_N_BYTE(p, n) (((unsigned int)p+n-1)&~(n-1))

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

int dmalloc(dm_t *dm, int size);
int dmfree(dm_t *dm);
int hw_xmit(unsigned long phy, int len);

#endif
