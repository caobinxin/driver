
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/leds.h>  
#include <linux/ctype.h>
#include <linux/platform_device.h>  
#include <plat/regs-gpio.h>
#include <mach/gpio.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/signal.h>
#include <asm/uaccess.h>
#include <plat/gpio-cfg.h>

#include "demo.h"

MODULE_AUTHOR("fgj");
MODULE_LICENSE("Dual BSD/GPL");

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

static struct led_classdev  led_dev;

static void led_classdev_set1(struct led_classdev *led_cdev, enum led_brightness value)  
{  
	led_cdev->brightness = value;
	if(value)
	{
		LED_SI_H(0);
		printk("__raw_readl(S3C64XX_GPMDAT)=0x%x\n",__raw_readl(S3C64XX_GPMDAT));
	}
	else
	{
		LED_SI_L(0);
		printk("__raw_readl(S3C64XX_GPMDAT)=0x%x\n",__raw_readl(S3C64XX_GPMDAT));
	}
}  
static enum led_brightness led_classdev_get1(struct led_classdev * led_cdev)  
{  
	return led_cdev->brightness; 
}

static void plate_test_release(struct device * dev)
{
    return ;
}

static struct platform_device plate_test_device = {
	.name   = "leddevtest",
	.id     = -1,
	.dev    = {
		.platform_data  = NULL,
		.release =plate_test_release,
	},
};

static int  plate_test_probe(struct platform_device *pdev)
{
    int ret;  
	led_dev.brightness_set = led_classdev_set1; 
	led_dev.brightness_get = led_classdev_get1;  
	led_dev.name = "fgjled";
	ret = led_classdev_register(&pdev->dev, &led_dev); 
	if (ret < 0) {  
		printk("led_classdev_register failed%d\n",ret);  
		return ret;  
	}  
	LED_SI_OUT1;
	LED_SI_OUT2;
	LED_SI_OUT3;
	LED_SI_OUT4;
	return 0;
}

static int  plate_test_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&led_dev);  
	return 0;
}

static struct platform_driver plate_test_driver = {
	.probe  = plate_test_probe,
	.remove = plate_test_remove,
	.driver = {
		.name   = "leddevtest",
		.owner  = THIS_MODULE,
	},
};

static int __init plateform_test_init (void)
{
	platform_device_register(&plate_test_device);
	platform_driver_register(&plate_test_driver);
	return 0;
}
static void __exit plateform_test_exit (void)
{
	platform_driver_unregister(&plate_test_driver);
	platform_device_unregister(&plate_test_device);
}
module_init (plateform_test_init);
module_exit (plateform_test_exit);

MODULE_DESCRIPTION("LED classdev driver example");  
MODULE_LICENSE("GPL"); 
