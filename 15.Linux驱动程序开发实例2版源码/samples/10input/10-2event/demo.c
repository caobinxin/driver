/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007, 2010 fengGuojin(fgjnew@163.com)
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <plat/regs-gpio.h>
#include <mach/gpio.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/signal.h>
#include <asm/uaccess.h>
#include <plat/regs-timer.h>
#include <plat/gpio-cfg.h>

MODULE_AUTHOR("fgj");
MODULE_DESCRIPTION("s3c6410 LED driver");
MODULE_LICENSE("GPL");

#define S3C64XX_GPM0_OUTPUT      (0x01 << 0)
#define S3C64XX_GPM1_OUTPUT      (0x01 << 4)
#define S3C64XX_GPM2_OUTPUT      (0x01 << 8)
#define S3C64XX_GPM3_OUTPUT      (0x01 << 12)

#define S3C64XX_GPMCON			(S3C64XX_GPM_BASE + 0x00)
#define S3C64XX_GPMDAT			(S3C64XX_GPM_BASE + 0x04)
#define S3C64XX_GPMPUD			(S3C64XX_GPM_BASE + 0x08)

#define LED_SI_OUT1	 s3c_gpio_cfgpin(S3C64XX_GPM(0),S3C64XX_GPM0_OUTPUT)
#define LED_SI_OUT2	 s3c_gpio_cfgpin(S3C64XX_GPM(1),S3C64XX_GPM1_OUTPUT)
#define LED_SI_OUT3	 s3c_gpio_cfgpin(S3C64XX_GPM(2),S3C64XX_GPM2_OUTPUT)
#define LED_SI_OUT4	 s3c_gpio_cfgpin(S3C64XX_GPM(3),S3C64XX_GPM3_OUTPUT)

#define LED_SI_H(i)	 __raw_writel(__raw_readl(S3C64XX_GPMDAT)|(1<<i),S3C64XX_GPMDAT)
#define LED_SI_L(i)	 __raw_writel(__raw_readl(S3C64XX_GPMDAT)&(~(1<<i)),S3C64XX_GPMDAT)

static char s3c6410_LED_name[] = "s3c6410LED";
static char s3c6410_LED_phys[] = "s3c6410LED";
static struct input_dev* s3c6410_LED_dev;

static int s3c6410_LED_event(struct input_dev *dev, unsigned int type, unsigned int code, int value)
{
	printk("s3c6410_LED_event type%d value%d\n",type,value);
	if (type != EV_LED)return -1;

	switch(value)
	{
		case 0:
			LED_SI_L(0);
			break;
		case 1:
			LED_SI_H(0);
			break;
	}
	return 0;
}

static int __init s3c6410_LED_init(void)
{
	LED_SI_OUT1;
	s3c6410_LED_dev=input_allocate_device();
	s3c6410_LED_dev->evbit[0] = BIT(EV_LED);
	s3c6410_LED_dev->ledbit[0] = BIT(LED_NUML);
	s3c6410_LED_dev->event = s3c6410_LED_event;

	s3c6410_LED_dev->name = s3c6410_LED_name;
	s3c6410_LED_dev->phys = s3c6410_LED_phys;
	s3c6410_LED_dev->id.bustype = BUS_HOST;
	s3c6410_LED_dev->id.vendor = 0x001f;
	s3c6410_LED_dev->id.product = 0x0001;
	s3c6410_LED_dev->id.version = 0x0100;

	input_register_device(s3c6410_LED_dev);
	printk(KERN_INFO "input: %s\n", s3c6410_LED_name);
	return 0;
}

static void __exit s3c6410_LED_exit(void)
{
    input_unregister_device(s3c6410_LED_dev);
}

module_init(s3c6410_LED_init);
module_exit(s3c6410_LED_exit);
