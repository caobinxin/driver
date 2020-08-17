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

#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define islower(c) ((c) >= 'a' && (c) <= 'z')
#define toupper(c) \
      (((c) >= 'a' && (c) <= 'z') ? ((char)('A'+((c)-'a'))) : (c))
#define isxdigit(c) \
      (isdigit(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))

typedef struct {
	int major;
	struct cdev testcdev;
	struct class *myclass;
	struct device *mydev;
} char_drv_info_t;
	  
typedef struct {
	unsigned int t;
	unsigned int l;
	unsigned char v[0];
} tlv_t;

typedef struct {
	unsigned int id;
	char *name;
	unsigned long def;
} arg_t;

typedef enum {
	ARG_ADDR,
	ARG_VALUE,
	ARG_SIZE,
	ARG_EN,
	M6X_ARG_MAX
} ARG_INDEX;

static arg_t cmd_args[M6X_ARG_MAX] = {
	{ARG_ADDR, "addr", 0},
	{ARG_VALUE, "value", 0},
	{ARG_SIZE, "size", 4},
	{ARG_EN, "en", 0},
};

spinlock_t lock;


arg_t *arg_index(unsigned int index)
{
	int i;

	for (i = 0; i < M6X_ARG_MAX; i++) {
		if (index == cmd_args[i].id) {
			return &cmd_args[i];
		}
	}
	return NULL;
}

unsigned long args_value(int index)
{
	int i;
	for (i = 0; i < M6X_ARG_MAX; i++) {
		if (index == cmd_args[i].id) {
			return cmd_args[i].def;
		}
	}
	return -1;
}

int cmdline_args(char *cmdline, char *cmd, arg_t * args, int argc);

int m6x_uart_tx_test(unsigned char *str, int len);
int m6x_uart_rx_work(void);

static struct kfifo rx_fifo;
int m6x_buffer_init(void)
{
	int ret;
	ret = kfifo_alloc(&rx_fifo, 1024*40, GFP_KERNEL);
	if(ret != 0)
	{
		printk(KERN_ERR "kfifo_alloc(8192) err\n");
		return -1;
	}
	printk(KERN_ERR "%s(%d, %d)\n", __func__, kfifo_size(&rx_fifo), kfifo_len(&rx_fifo));
	return 0;
}
int ioctl_trigger(void);
int m6x_buffer_put(unsigned char *buf, int len)
{
	int ret;
	unsigned long flag;
	spin_lock_irqsave(&lock, flag);
	ret = kfifo_in(&rx_fifo, buf, len);
	//printk("m6x_buffer_put:%s\n", buf);
	//hexdump("m6x_buffer_put", buf, len);
	spin_unlock_irqrestore(&lock, flag);
	ioctl_trigger();
	return 0;
}

int m6x_buffer_get(unsigned char *buf, int *len)
{
	int ret;
	unsigned long flag;
	spin_lock_irqsave(&lock, flag);
	if(*len > kfifo_len(&rx_fifo))
		*len = kfifo_len(&rx_fifo);
	if(*len <= 0)
	{
		*len = 0;
		spin_unlock_irqrestore(&lock, flag);
		 return -1;
	}
	ret =  kfifo_out(&rx_fifo, buf, *len);
	//hexdump("m6x_buffer_get", buf, *len);
	spin_unlock_irqrestore(&lock, flag);
	return 0;
}

int m6x_buffer_len(void)
{
	return kfifo_len(&rx_fifo);
}

int m6x_regs_dump(void);
int tx_test(int value);
int rx_test(int addr, int size);
int char_no_zero(unsigned char *dst, unsigned char *src, int *len);
int m6x_uart_rx_test(void);
int m6x_pps_1us(int value);
int m6x_pps_16ns(int value);
int m6x_pps_en(int value);

int char_ioctl_cmd(unsigned char *input, int input_len, tlv_t * replay)
{
	int ret;
	dm_t dm;
	unsigned long addr, value, size, en;
	unsigned char cmd[64];

	memset(cmd, 0x00, sizeof(cmd));
	memset(replay->v, 0x00, 1024);
	
	hexdump("input", input, 64);
	printk(KERN_ERR "%s:%s\n", __func__, input);
	if (0 != memcmp(input, "linux", 5) ) {
		
		printk(" 回车进入\n") ;
		printk("strlen(input) = %d\n", strlen(input)) ;
		printk("input = %s\n", input) ;

		m6x_uart_tx(input, strlen(input), 1000);
		return 0;
	}
	input += 6;
	ret = cmdline_args(input, cmd, cmd_args, M6X_ARG_MAX);
	if (0 != ret) {
		printk(KERN_ERR "cmdline_args err\n");
		return -1;
	}
	printk(KERN_ERR "cmd:%s\n", cmd);
	if (0 == strcmp("readmem", cmd)) {
		addr = args_value(ARG_ADDR);
		size = args_value(ARG_SIZE);
		replay->t = 1;
		replay->l = size;
		hwcopy(replay->v, (unsigned char *)addr, replay->l);
	} else if (0 == strcmp("writemem", cmd)) {
		addr = args_value(ARG_ADDR);
		value = args_value(ARG_VALUE);
		size = args_value(ARG_SIZE);
		printk(KERN_ERR "memwrite addr=0x%lx value=0x%lx size=%ld\n", addr, value, size);
		hwcopy((char *)(addr), (char *)&value, 4);
	} else if (0 == strcmp("readbar", cmd)) {
		addr  = args_value(ARG_ADDR);
		size  = args_value(ARG_SIZE);
		replay->t = 1;
		replay->l = size;
		memset(replay->v, 0x00, (int)size);
		pcie_bar_read(0, (unsigned int)addr, replay->v, (int)size);
		hexdump("readbar", replay->v, size);
		printk(KERN_ERR "%s\n", replay->v);
	} else if (0 == strcmp("writebar", cmd)) {
		addr  = args_value(ARG_ADDR);
		size  = args_value(ARG_SIZE);
		value = args_value(ARG_VALUE);
		replay->t = 1;
		replay->l = size;
		pcie_bar_write(0, (unsigned int )addr, (unsigned char *)(&value), 4);
	} else if (0 == strcmp("tod", cmd)) {
		en    = args_value(ARG_EN);
		printk(KERN_ERR "tod en = %ld\n", en);
		m6x_tod_display_set(en);
		replay->t = 0;
		replay->l = 3;
		strcpy(replay->v, "ok");
	} else if (0 == strncmp("GPRMC", cmd, 5)) {
		replay->t = 0;
		m6x_pps_tod_get(0, replay->v, &(replay->l));
	} else if (0 == strncmp("GPZDA", cmd, 5)) {
		replay->t = 0;
		m6x_pps_tod_get(1, replay->v, &(replay->l));
	} else if (0 == strncmp("UTC", cmd, 3)) {
		m6x_pps_tod_get(2, replay->v, &(replay->l));
	} else if (0 == strcmp("uart-rx", cmd)) {
		replay->t = 0;
		m6x_uart_rx_work();
	} else if (0 == strncmp("uart-tx", cmd, 7)) {
		size  = args_value(ARG_SIZE);
		value = args_value(ARG_VALUE);
		input[0] = *(unsigned char *)(&value);
		m6x_uart_tx_test((unsigned char *)(input), size);
	} else if (0 == strncmp("uart-reset", cmd, 10)) {
		m6x_uart_reset();
	} else if (0 == strncmp("regdump", cmd, 7)) {
		m6x_regs_dump();
	} else if (0 == strncmp("regwrite", cmd, 8)) {
		addr  = args_value(ARG_ADDR);
		value = args_value(ARG_VALUE);
		m6x_reg_write((unsigned int )addr, (unsigned int )value);
	} else if (0 == strncmp("dmalloc", cmd, 7)) {
		size  = args_value(ARG_SIZE);
		dmalloc(&dm, size);
	} else if (0 == strncmp("tx-test", cmd, 7)) {
		value = args_value(ARG_VALUE);
		tx_test(value);
	} else if (0 == strncmp("rx-test", cmd, 7)) {
		addr  = args_value(ARG_ADDR);
		size  = args_value(ARG_SIZE);
		rx_test(addr, size);
	} else if (0 == strncmp("uart-rx-test", cmd, 12)) {
		m6x_uart_rx_test();
	} else if (0 == strncmp("1us", cmd, 3)) {
		value = args_value(ARG_VALUE);
		m6x_pps_1us(value);
	} else if (0 == strncmp("16ns", cmd, 4)) {
		value = args_value(ARG_VALUE);
		m6x_pps_16ns(value);
	} else if (0 == strncmp("pps", cmd, 3)) {
		value = args_value(ARG_VALUE);
		m6x_pps_en(value);
	} else {
		printk(KERN_ERR "unknow cmd %s\n", cmd);
		hexdump("unknow cmd", cmd, strlen(cmd)+16 );
	}
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

int hex2char(unsigned int *arry, int len)
{

	int i,j,p,l;
	unsigned char buf[256];

	printk(KERN_ERR "%s(%d)\n", __func__, len);
    if(len < 32)
	{
		printk(KERN_ERR "line[0]:%s\n", (unsigned char *)arry);
		return 0;
	}
	len = len/4;
	memset(buf, 0x00, sizeof(buf));
	for(i=0,j=0,p=0,l=0; i < len; i++)
	{
		snprintf(buf+p, 5, "%s", (char *)(&arry[i]));
		p += 4;
		//snprintf(buf+p, 2, ",");
		//p += 1;
		j++;
		if(j == 8)
		{
			printk(KERN_ERR "line[%d]:%s\n", l, buf);
			memset(buf, 0x00, sizeof(buf));
			j = p = 0;
			l++;
		}
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

int cmdline_args(char *cmdline, char *cmd, arg_t * args, int argc)
{
	int status = 0;
	char *end;
	char *argname;
	char *cp;
	int DONE;
	int FOUND = 0;

	unsigned short base;
	unsigned long result, value;
	unsigned long val;
	unsigned long i;
	unsigned long j;

	for (i = 0; i < argc; i++) {
		args[i].def = 0;
	}
	while (*cmdline == ' ' || *cmdline == '\t')
		cmdline++;

	while (*cmdline != ' ' && *cmdline != '\t' && *cmdline != '\0') {
		*cmd = *cmdline;
		cmd++;
		cmdline++;
	}
	*cmd = '\0';
	if (*cmdline == '\0') {
		goto WEDONE;
	}

	*cmdline = '\0';
	cmdline++;
	while (*cmdline == ' ' || *cmdline == '\t')
		cmdline++;

	end = cmdline;
	while (*end == ' ' || *end == '\t')
		end++;
	DONE = (*end == '\0') ? 1 : 0;
	while (!DONE) {
		while (*end != '=' && *end != '\0')
			end++;
		if (*end == '\0') {
			status = 1;
			goto WEDONE;
		}
		*end = '\0';
		argname = cmdline;
		cmdline = ++end;
		if (*end == ' ' || *end == '\t' || *end == '\n') {
			status = 1;
			goto WEDONE;
		}
		while (*end != ' ' && *end != '\t' && *end != '\n'
		       && *end != '\0')
			end++;
		if (*end == '\0')
			DONE = 1;
		else
			*end = '\0';

		if (!strcmp(argname, "file") || !strcmp(argname, "filec")) {
			val = 1;
		} else {
			val = 0;
			result = 0;
			cp = cmdline;
			if (cp[0] == '0' && (cp[1] == 'x' || cp[1] == 'X')) {
				base = 16;
				cp += 2;

			} else {
				base = 10;
			}
			while (isxdigit(*cp)) {
				value = isdigit(*cp) ? (*cp - '0')
				    : ((islower(*cp) ? toupper(*cp) : *cp) -
				       'A' + 10);

				result = result * base + value;
				cp++;
			}

			val = result;
		}

		FOUND = 0;
		for (j = 0; j < argc && !FOUND; j++) {
			if (!strcmp(argname, args[j].name)) {
				args[j].def = val;
				FOUND = 1;
			}
		}
		if (!FOUND) {
			printk(KERN_ERR "arg %s err\n", argname);
			status = 0;
			goto WEDONE;
		}
		cmdline = ++end;
		while (*cmdline == ' ' || *cmdline == '\t' || *cmdline == '\n')
			cmdline++;
		end = cmdline;
		if (*end == '\0')
			DONE = 1;
	}
WEDONE:
	return status;
}

char_drv_info_t char_drv_info = { 0 };

int char_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ERR "%s\n", __func__);
	return 0;
}

ssize_t char_write(struct file * filp, const char __user * buffer, size_t count,
		   loff_t * offset)
{
	printk(KERN_ERR "%s\n", __func__);
	return 0;
}

ssize_t char_read(struct file * filp, char __user * buffer, size_t count,
		  loff_t * offset)
{
	printk(KERN_ERR "%s\n", __func__);
	return 0;
}

int char_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ERR "%s\n", __func__);
	return 0;
}

long char_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret;
	tlv_t *tlv;
	unsigned char *buf[2];

	printk(KERN_ERR "%s(%d)\n", __func__, cmd);
	buf[0] = kmalloc(4096, GFP_KERNEL);
	buf[1] = kmalloc(4096, GFP_KERNEL);

	memset(buf[1], 0x00, 4096);
	tlv = (tlv_t *) buf[1];

	ret = copy_from_user(buf[0], (char __user *)arg, 64);

	switch(cmd)
	{
	case 1:
		char_ioctl_cmd(buf[0], 4096, (tlv_t *)buf[1]);
		break;
	case 4:
		tlv->l = 4092;
		m6x_buffer_get(tlv->v, &(tlv->l));
		
		tlv->t = 0;
		break;
	default:
		break;
	}
	ret = copy_to_user((char __user *)arg, buf[1], sizeof(tlv_t) + tlv->l);

	kfree(buf[0]);
	kfree(buf[1]);
	return 0;
}

static struct fasync_struct *button_async; 
static int char_ioctl_fasync (int fd, struct file *filp, int on)  
{  
    printk(KERN_ERR "%s\n", __func__);  
    return fasync_helper (fd, filp, on, &button_async);  
}

int ioctl_trigger(void)
{
	kill_fasync (&button_async, SIGIO, POLL_IN);  
	return 0;
}

static struct file_operations fop = {
	.owner = THIS_MODULE,
	.open = char_open,
	.release = char_release,
	.write = char_write,
	.read = char_read,
	.unlocked_ioctl = char_ioctl,
	.fasync  =  char_ioctl_fasync,
};

struct timer_list ioctl_timer;
struct timer_list uralt_timer;
struct timer_list uart_inter_timer;
void uart_inter_set_timer_func(unsigned long data)
{
	unsigned int value;	
	pcie_bar_read(0, 0x48,  (unsigned char *)&value, 4);
	if( value & ( 1 << 3)){
		printk("uart interrupt enable ok... \n") ;
		return ;
	}
	
	m6x_reg_bit_set(0x48, 3);
	printk("uart interrupt disable , 1s ...\n") ;

	mod_timer(&(uart_inter_timer), jiffies + msecs_to_jiffies(1000));
}

void ioctl_timer_func(unsigned long data)
{
	m6x_uart_rx_work();
	/*
	if(m6x_buffer_len() > 0)
	{
		ioctl_trigger();	
	}
	*/
	mod_timer(&(ioctl_timer), jiffies + msecs_to_jiffies(10));
	return;
}

int m6x_ioctl_init(void)
{
	int ret;
	dev_t dev;
	char_drv_info_t *drv = &char_drv_info;

	drv->major = 321;
	dev = MKDEV(drv->major, 0);
	ret = register_chrdev_region(dev, 1, "char");
	if (ret) {
		alloc_chrdev_region(&dev, 0, 1, "char");
		drv->major = MAJOR(dev);
	}
	drv->testcdev.owner = THIS_MODULE;
	cdev_init(&(drv->testcdev), &fop);
	cdev_add(&(drv->testcdev), dev, 1);

	drv->myclass = class_create(THIS_MODULE, "char_class");
	drv->mydev = device_create(drv->myclass, NULL, dev, NULL, "m6x");

	spin_lock_init(&lock);
	m6x_buffer_init();
	
	ioctl_timer.function = ioctl_timer_func;
	init_timer(&ioctl_timer);
	//mod_timer(&(ioctl_timer), jiffies + msecs_to_jiffies(1000));
	
	uralt_timer.function = m6x_uart_rx_work;
	init_timer(&uralt_timer);
	mod_timer(&(uralt_timer), jiffies + msecs_to_jiffies(1000));


	uart_inter_timer.function = uart_inter_set_timer_func ;
	init_timer(&uart_inter_timer);
	mod_timer(&(uart_inter_timer), jiffies + msecs_to_jiffies(1000));

	printk(KERN_ERR "module init ok ...\n");
	return 0;
}

void m6x_ioctl_exit(void)
{
	dev_t dev;
	char_drv_info_t *drv = &char_drv_info;

	dev = MKDEV(drv->major, 0);

	device_destroy(drv->myclass, dev);
	class_destroy(drv->myclass);

	cdev_del(&(drv->testcdev));
	unregister_chrdev_region(dev, 1);

	del_timer_sync(&ioctl_timer);
	
	printk(KERN_ERR "module exit ok....\n");
	return;
}
