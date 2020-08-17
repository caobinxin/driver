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

typedef struct
{
	unsigned short sync;
	unsigned char  class;
	unsigned char  id;
	unsigned char  len[2];
}__attribute__ ((packed)) tod_hdr_t;

typedef struct
{
	tod_hdr_t      hdr;
	unsigned int   tow;
	unsigned int   rsvd1;
	unsigned short week;
	unsigned char  leaps;
	unsigned char  state;
	unsigned char  tacc;
	unsigned char  rsvd2[3];
	unsigned char  fcs;
}__attribute__ ((packed)) tod_time_info_t;

typedef struct
{
	tod_hdr_t      hdr;
	unsigned char  type;
	unsigned short source;
	unsigned short monitor;
	unsigned char  rsvd1[11];
	unsigned char  fcs;
}__attribute__ ((packed)) tod_time_state_t;

extern int do_settimeofday(const struct timespec *tv);
extern void do_gettimeofday(struct timeval *tv);
extern int rtc_tm_to_time(struct rtc_time *tm, unsigned long *time);
extern void rtc_time_to_tm(unsigned long time, struct rtc_time *tm);

char m6x_pps_buffer[2][0x84];
char m6x_tod_buffer[5][254];

int m6x_tod_display = 1;

int m6x_tod_display_set(int en)
{
	m6x_tod_display = en;
	return 0;
}
int ioctl_trigger(void);
int m6x_buffer_len(void);

int pps_tod_to_time(unsigned char *str, struct rtc_time *tm)
{
	unsigned char *p;
	unsigned int data[6];
	//printk(KERN_ERR "INPUT:%s\n", str);
	if(strncmp(str, "$GPRMC", 6) == 0)
	{
		sscanf(str, "$GPRMC,%d,A,,,,,,,%d,,*%d", &data[0], &data[1], &data[2]);
		tm->tm_hour = data[0]/10000;
		tm->tm_min	= (data[0]/100)%100;
		tm->tm_sec	= data[0]%100;
		tm->tm_mday = data[1]/10000;
		tm->tm_mon	= (data[1]/100)%100;
		tm->tm_year = data[1]%100 + 2000;
		sprintf(m6x_tod_buffer[0], "%s(%d-%d-%d %d:%d:%d)\n", str, tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		if(m6x_tod_display)
		{
			printk(KERN_ERR "put %s\n", m6x_tod_buffer[0]);
			m6x_buffer_put(m6x_tod_buffer[0], strlen(m6x_tod_buffer[0]));
			/*
			if(m6x_buffer_len() > 0)
			{
				ioctl_trigger();	
			}
			*/
		}
	}
	else if(strncmp(str, "$GPZDA", 6) == 0)
	{
		sscanf(str, "$GPZDA,%d,%d,%d,%d,,*%d", &data[0], &data[1], &data[2], &data[3], &data[4]);
		tm->tm_hour = data[0]/10000;
		tm->tm_min	= (data[0]/100)%100;
		tm->tm_sec	= data[0]%100;
		tm->tm_mday = data[1];
		tm->tm_mon	= data[2];
		tm->tm_year = data[3];
		sprintf(m6x_tod_buffer[1], "%s(%d-%d-%d %d:%d:%d)\n", str, tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		if(m6x_tod_display)
		{
			m6x_buffer_put(m6x_tod_buffer[1], strlen(m6x_tod_buffer[1]));
		}
		printk(KERN_ERR "%s\n", m6x_tod_buffer[1]);
	}
	else if(strncmp( str+20, "UTC", 3) == 0)
	{
		sscanf(str, "%d-%d-%d %d:%d:%d UTC", &data[0], &data[1], &data[2], &data[3], &data[4], &data[5]);
		tm->tm_year = data[0];
		tm->tm_mon  = data[1];
		tm->tm_mday = data[2];
		tm->tm_hour = data[3];
		tm->tm_min  = data[4];
		tm->tm_sec  = data[5];
		p = strncpy(m6x_tod_buffer[2], str, 23);
		sprintf(m6x_tod_buffer[2]+23, "(%d-%d-%d %d:%d:%d)\n",tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		if(m6x_tod_display)
		{
			m6x_buffer_put(m6x_tod_buffer[2], strlen(m6x_tod_buffer[2]));
		}
		printk(KERN_ERR "%s\n", m6x_tod_buffer[2]);
	}
	else
	{
		//printk(KERN_ERR "%s err\n", __func__);
		return -1;
	}
	tm->tm_wday = 4; 
	tm->tm_yday = 285;
	tm->tm_isdst = 0;
	return 0;
}

int pps_pretimer_set(int ms)
{
	return m6x_reg_write(0x340, ms<<1);
}

int pps_polling(int ms)
{
	unsigned int value;
	do
	{
		m6x_reg_read(0xff, &value);
		if(value & (1<<1))
			return 0;
	}
	while(ms--);
	return -1;
}

int tvsec_to_tm(unsigned long tvsec, struct rtc_time *tm)
{
	//do_gettimeofday(&tvsec);
	printk(KERN_ERR "%s(%ld)\n", __func__, tvsec);

	tvsec -= sys_tz.tz_minuteswest * 60;
	rtc_time_to_tm(tvsec, tm);
	
	tm->tm_year += 1900;
	tm->tm_mon  += 1;
	tm->tm_hour += 8;

   	printk(KERN_ERR "%04d-%02d-%02d %02d:%02d:%02d (%d:%d:%d)\n"
        ,tm->tm_year
        ,tm->tm_mon
        ,tm->tm_mday
        ,tm->tm_hour
        ,tm->tm_min
        ,tm->tm_sec
		,tm->tm_wday
		,tm->tm_yday
		,tm->tm_isdst);
	return 0;
}

int tm_to_tvsec(struct rtc_time *tm, unsigned long *tvsec)
{
	tm->tm_year -= 1900;
	tm->tm_mon  -= 1;
	tm->tm_hour -= 8;

	rtc_tm_to_time(tm, tvsec);
	*tvsec += sys_tz.tz_minuteswest * 60;

	//printk(KERN_ERR "%s(%ld)\n", __func__, *tvsec);
	return 0;
}

int m6x_pps_tod_get(int id, unsigned char *report, int *len)
{
	if(id>2)
	{
		printk(KERN_ERR "%s(%d) err\n", __func__, id);
		*len = 0;
	}
	
	*len = strlen(m6x_tod_buffer[id]);
	strcpy(report, m6x_tod_buffer[id]);
	//printk(KERN_ERR "%s(%d, %d) ok\n", __func__, id, *len);
	return 0;
}

unsigned char m6x_pps_interrupt_buffer[1024];

int m6x_pps_interrupt(void)
{
	int ret, len;
	struct timespec   ts = {0};//tv_sec tv_nsec
	struct rtc_time   tm[2];;

	m6x_reg_read(0xa0, &len);
	if(len <= 0)
		return -1;
	memset(m6x_pps_interrupt_buffer, 0x00, sizeof(m6x_pps_interrupt_buffer));
	ret = pcie_bar_read(0, 0x2000,    m6x_pps_interrupt_buffer, len);
	m6x_reg_bit_clr(0xa8, 0);
	m6x_reg_bit_set(0xa8, 0);

	//hexdump("pps", m6x_pps_interrupt_buffer, len);
	//printk(KERN_ERR "PPS:%s\n", m6x_pps_interrupt_buffer);
	pps_tod_to_time(m6x_pps_interrupt_buffer+1, &tm[0]);
	return 0;
	ret = pcie_bar_read(0, 0x60+0,    m6x_pps_buffer[0], 0x80);//0x80
	ret = pcie_bar_read(0, 0x60+0x80, m6x_pps_buffer[1], 0x80);//0x80

	pps_tod_to_time(m6x_pps_buffer[0], &tm[0]);
	
	if(strncmp( m6x_pps_buffer[0]+20, "UTC", 3) != 0)
	{
		pps_tod_to_time(m6x_pps_buffer[1], &tm[1]);
	}
	tm_to_tvsec(&tm[0], &ts.tv_sec);
	//do_settimeofday(&ts);
	return 0;
}

int m6x_pps_1us(int value)
{
	m6x_reg_write(0x10c, value);
	m6x_reg_write(0x114, 0);
	return 0;
}

int m6x_pps_16ns(int value)
{
	m6x_reg_write(0x110, value);
	m6x_reg_write(0x114, 1);
	return 0;
}

int m6x_pps_en(int value)
{
	if(value)
		m6x_reg_bit_set(0x048,1);
	else
		m6x_reg_bit_clr(0x048,1);
	return 0;
}

int m6x_pps_init(void)
{
	m6x_reg_bit_set(0x48, 1);
	return 0;
}

