#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/watchdog.h>


int main(int argc, const char *argv[])
{
	int fd=open("/dev/watchdog",O_WRONLY);
	if (fd==-1) 
	{
		perror("watchdog"); return -1;
	}
	int timeout =4;
	ioctl(fd, WDIOC_SETTIMEOUT, &timeout);

	while(1) 
	{
		ioctl(fd, WDIOC_KEEPALIVE, 0);
		sleep(1);
	}
	close(fd);
	return 0;
}
