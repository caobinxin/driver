
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[])
{
	int i, fd, fbfd;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	__u8 *fb_buf,*fbptr;
	int fb_xres,fb_yres,fb_bpp;
	__u32 screensize;

	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd < 0) {
		fbfd = open("/dev/fb/0", O_RDWR);
		if(fbfd<0) {
			printf("Error: cannot open framebuffer device.\n");
		        return -1;
		}
	}

	// Get fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {		
		printf("Error reading fixed information.\n");
		close(fbfd);
        	return -1;
	}

	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable information.\n");
		close(fbfd);
		return -1;
	}

	printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel );
	fb_xres = vinfo.xres;
	fb_yres = vinfo.yres;
	fb_bpp  = vinfo.bits_per_pixel;

	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	fb_buf = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
				fbfd, 0);
	if ((int)fb_buf == -1) {
		printf("Error: failed to map framebuffer device to memory.\n");
		close(fbfd);
        	return -1;
	}
	fbptr=fb_buf;
	for(i=0;i<vinfo.yres;i++)
	{
		memset(fbptr,i&0xFF,vinfo.xres * vinfo.bits_per_pixel / 8);
		fbptr+=vinfo.xres * vinfo.bits_per_pixel / 8;
	}
	printf("ummap framebuffer device to memory.\n");
	sleep(10);
	munmap(fb_buf, screensize);
	close(fbfd);

	return 0;
}
