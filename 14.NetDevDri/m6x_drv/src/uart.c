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

int char_no_zero(unsigned char *dst, unsigned char *src, int *len)
{
	int i,j;
	for(i=j=0; i < (*len); i++)
	{
		if(src[i] == 0)
			continue;
		dst[j++] = src[i];
	}
	*len = j;
	return 0;
}

int m6x_uart_rx_test(void)
{
	unsigned char buf1[0x7ff];
	unsigned char buf2[0x7ff];
	
	memset(buf1, 0x00, sizeof(buf1));
	memset(buf2, 0x00, sizeof(buf2));
	
	pcie_bar_read(0, (unsigned int)0x2000, buf1, 0x7ff);
	char_no_zero(buf2, buf1, 255);
	hexdump("read 0x1000", buf1, 255);
	printk(KERN_ERR "buf:%s\n", buf2);
	return 0;
}

int m6x_uart_tx(unsigned char *buf, int len, int timeout)
{
	unsigned int value[2];
	static int uart_tx_len = 0;

	buf[len-1] = 0x0d;
	//buf[len++] = 0x0a;
	memset(buf+len, 0x00, 4);
	printk("----------------------\n") ;
	printk(KERN_ERR "%s len %d %s start...\n", __func__, len, buf);
	printk("----------------------\n") ;
	if(len > (0x7ff - 0x3ff))
	{
		printk(KERN_ERR "%s len %d err\n", __func__, len);
		return -1;
	}
	hexdump("m6x_uart_tx", buf, len);
	/*下行数据为 6000*/
#if 1
	m6x_mreg_write(0x6000, buf, ALIGN_N_BYTE(len, 4));
#else
	m6x_mreg_write(0x4000, buf, ALIGN_N_BYTE(len, 4));
#endif
	m6x_reg_write(0x90,len);
	
	m6x_reg_bit_clr(0x94, 0);
	m6x_reg_bit_set(0x94, 0);
	printk(KERN_ERR "%s done\n", __func__);


	/*
	 * 测试使用：
	 * */

	//m6x_uart_rx_work() ;


	return 0;
}

int m6x_uart_tx_test(unsigned char *str, int len)
{
	int i;
	unsigned char c = str[0];

	len = ALIGN_N_BYTE(len, 4);
	memset(str, 0x00, len+4);
	for(i=0; i<len; i++)
	{
		str[i] = c;
		if(c == 'Z')
			c = 'A';
		else
			c += 1;
	}
	m6x_uart_tx( str, len, 10);
	return 0;
}
int m6x_uart_rx_work(void)
{
	int value, ret, len = 0;
	unsigned char *tmp[2];

	m6x_reg_read(0x84, &value);
	if(!(value & 0x01))
	{
		printk(KERN_ERR "reg[0x84]:0x%x err\n", value);
		printk("FPGA 没有串口数据上报\n") ;
		return -1;
	}
	m6x_reg_read(0x80, &len);
	printk(KERN_ERR "uart rx len:%d\n", len);
	if(len <= 0)
	{
		return -1;
	}
	tmp[0] = kmalloc(len+4, GFP_KERNEL);
	tmp[1] = kmalloc(len+4, GFP_KERNEL);

#if 1 /*上行数据是 4000*/
	pcie_bar_read(0, 0x4000, tmp[0], len);
	m6x_reg_bit_set(0x88, 0);
	m6x_reg_bit_clr(0x88, 0);
#else
	pcie_bar_read(0, 0x6000, tmp[0], len);
	m6x_reg_bit_clr(0x88, 0);
	m6x_reg_bit_set(0x88, 0);
#endif

	hexdump("uart-rx", tmp[0], len);
	printk("--------------uart-rx -------------\n") ;

	char_no_zero(tmp[1], tmp[0], &len);
	m6x_buffer_put(tmp[1], len);
	kfree(tmp[0]);
	kfree(tmp[1]);

	return 0;
}

int m6x_uart_rx_work__(void)
{
	int ret;
	unsigned char *tmp[2];
	int len = 0;
	
	ret = m6x_reg_read(0x394, &len);
	if(len <= 0)
	{
		//printk(KERN_ERR "uart-rx(%d)\n", len);
		return -1;
	}
	if(len >= 2048)
	{
		printk(KERN_ERR "%s len %d err\n", __func__, len);
		len = 2044;
	}
	
	tmp[0] = kmalloc(len+4, GFP_KERNEL);
	tmp[1] = kmalloc(len+4, GFP_KERNEL);
	memset(tmp[0], 0x00, len+4);
	memset(tmp[1], 0x00, len+4);

	m6x_reg_write(0x390, len);
	m6x_reg_bit_set(0x384, 16);
	pcie_bar_read(0, 0x800, tmp[0], len);
	if(len <= 40)
		udelay(1);
	else
		udelay(len/40);
	m6x_reg_bit_set(0x384, 24);
	m6x_reg_write(0x390, 0);
	m6x_reg_bit_clr(0x384, 16);
	m6x_reg_bit_clr(0x384, 24);

	//hex2char((unsigned int *)(tmp[0]),len);

	char_no_zero(tmp[1], tmp[0], &len);

	m6x_buffer_put(tmp[1], len);

	kfree(tmp[0]);
	kfree(tmp[1]);
	return 0;
}

int m6x_uart_reset(void)
{
    printk(KERN_ERR "%s\n", __func__);
	//m6x_reg_bit_set(0x384, 31);
	//m6x_reg_bit_clr(0x384, 31);
	return 0;
}

int m6x_uart_init(void)
{
	m6x_reg_bit_set(0x48, 3);
	return 0;
}


