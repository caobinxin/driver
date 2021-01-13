#include<sys/types.h>
#include<unistd.h>
#include<fcntl.h>
#include<linux/rtc.h>
#include<linux/ioctl.h>
#include<stdio.h>
#include<stdlib.h>

main()
{
  int fd;
  int i;
  char data[256];
  
  int retval;
  fd=open("/dev/fgj",O_RDWR);
  if(fd==-1)
  {
     perror("error open\n");
     exit(-1);
  }
  printf("open /dev/fgj successfully\n");
  
  memset(data,0,256);
  retval=read(fd,data,255);
  if(retval==-1)
  {
    perror("read error\n");
    exit(-1);
  }
  printf("%s\n",data);

  close(fd);
}
