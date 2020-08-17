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
	m6x_reg_bit_set(0x28, 8);
	return 0;
}



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
		flag = 1;
	}
	for(i=0; i<(512/4); i++)
	{
		dword[i] = value++;
	}
	for(i=0; i<256; i++)
		buf[i] = i;
	hexdump("tx_test", buf, 256);
	hw_xmit(dm.phy, 256);
	return 0;
}

int rx_test(int addr, int size)
{
	int i;
	static int flag = 0;
	static dm_t dm;
	static unsigned int  *dword;
	static unsigned char *buf;
	if(flag == 0)
	{
		dmalloc(&dm, 4096);
		dword  = (unsigned int  *)(dm.vir);
		buf    = (unsigned char *)(dm.vir);
		flag = 1;
	}
	memset(buf, 0x00, 4096);
	printk(KERN_ERR "phy(0xl%x, 0xl%x), virt(0x%lx, 0x%lx)\n", dm.phy, virt_to_phys(dm.vir), dm.vir, phys_to_virt(dm.phy) );
	m6x_reg_write(0x38, (unsigned int)(dm.phy) + addr);
	m6x_reg_write(0x3c, (unsigned int)(dm.phy>>32));
	m6x_reg_write(0xac, size);
	m6x_reg_bit_set(0x28, 0);
	hexdump("rx_test", buf+addr, size);
	return 0;
}
