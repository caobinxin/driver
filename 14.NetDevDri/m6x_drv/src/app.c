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

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>


#define ALIGN_N_BYTE(p, n) (((unsigned int)p+n-1)&~(n-1))

typedef struct {
	unsigned int t;
	unsigned int l;
	unsigned char v[0];
} tlv_t;

static int g_fd;
unsigned char main_buffer[4096];
unsigned char sign_buffer[4096];

int hexdump(char *name, char *buf, int len)
{
	int i, count;
	unsigned int *p;
	count = len / 32;
	count += 1;
	printf("hexdump %s hex(len=%d):\n", name, len);
	for (i = 0; i < count; i++) {
		p = (unsigned int *)(buf + i * 32);
		printf
		    ("mem[0x%04x] 0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,\n",
		     i * 32, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
	}
	return 0;
}

int run_ioctl(int fd, int cmd, unsigned char *buf)
{
	int ret;
	tlv_t *tlv = (tlv_t *)buf;
	//printf("%s(%d, %s)\n", __func__, cmd, buf);
	ret = ioctl(fd, cmd, buf);
	if (ret < 0) {
		printf("ioctl ret %d err\n", ret);
		//close(fd);
		return -1;
	}
	switch (tlv->t) {
	case 0:
		if(tlv->l <= 0)
			return 0;
		//hexdump("replay", tlv->v, tlv->l);
		printf("%s\n", tlv->v);
		break;
	case 1:
		hexdump("replay", tlv->v, tlv->l);
		break;
	default:
		break;
	}
	return 0;
}

void app_signal_func(int sig)  
{
	//printf("-->\n");
	memset(sign_buffer, 0x00, sizeof(sign_buffer));
	run_ioctl(g_fd, 4, sign_buffer);
	return;
}

static long time_d_ms(struct timeval *start, struct timeval *end)
{
	return (end->tv_sec - start->tv_sec)*1000 + (end->tv_usec - start->tv_usec)/1000;
}

int main(int argc, char *argv[])
{
	int i, flag, ret, len = 0 ;
	unsigned char *p;
	struct timeval tv[2];

	memset((unsigned char *)tv, 0x00, sizeof(tv));
	p = main_buffer;
	memset(main_buffer, 0x00, sizeof(main_buffer));
	for (i = 1; i < argc; i++) {
		p += sprintf(p, " %s", argv[i]);
	}
	g_fd = open("/dev/m6x", O_RDWR);
	if ( g_fd < 0 ) {
		printf("open err\n");
		printf("Are you sure \n\tsudo insmode driver ?\n\tsudo ./a.out ?\n") ;
		return -1;
	}
	printf("/dev/m6x open success\n") ;

	signal(SIGIO, app_signal_func);

	ret = fcntl(g_fd, F_SETOWN, getpid());
	flag = fcntl(g_fd, F_GETFL);
	flag = fcntl(g_fd, F_SETFL, flag | FASYNC);
	
	while(1)
	{
		memset(main_buffer, 0x00, sizeof(main_buffer));
		fgets(main_buffer, sizeof(main_buffer), stdin);
		fflush(stdin);
		gettimeofday(&tv[1], NULL);
		time_d_ms(&tv[0], &tv[1]);
		if(time_d_ms(&tv[0], &tv[1]) < len*2)
			continue;
		
		run_ioctl(g_fd, 1, main_buffer);
		len = strlen(main_buffer)+2;
		gettimeofday(&tv[0], NULL);
	}
	close(g_fd);
	return 0;
}

