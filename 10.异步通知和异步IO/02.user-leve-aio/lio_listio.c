#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<fcntl.h>
#include<aio.h>
 

/**
 * lio_listio()函数可用于同时发起多个传输。这个函数非常的重要，它使得用户可以在一个系统调用中启动大量的io操作
 * 
 * */ 
#define BUFFER_SIZE 1025
 
int MAX_LIST = 2;
 
 
int main(int argc,char **argv)
{
    struct aiocb *listio[MAX_LIST];
    struct aiocb rd,wr;
    int fd,ret;
 
    //异步读事件
    fd = open("test1.txt",O_RDONLY);
    if(fd < 0)
    {
        perror("test1.txt");
    }
 
    bzero(&rd,sizeof(rd));
 
    rd.aio_buf = (char *)malloc(BUFFER_SIZE);
    if(rd.aio_buf == NULL)
    {
        perror("aio_buf");
    }
 
    rd.aio_fildes = fd;
    rd.aio_nbytes = 1024;
    rd.aio_offset = 0;
    rd.aio_lio_opcode = LIO_READ;   ///lio操作类型为异步读
 
    //将异步读事件添加到list中
    listio[0] = &rd;
 
 
    //异步些事件
    fd = open("test2.txt",O_WRONLY | O_APPEND);
    if(fd < 0)
    {
        perror("test2.txt");
    }
 
    bzero(&wr,sizeof(wr));
 #if 0
    wr.aio_buf = (char *)malloc(BUFFER_SIZE);
    wr.aio_nbytes = 1024;
#else
    wr.aio_buf = "caobinxin";
    wr.aio_nbytes = strlen(wr.aio_buf);
    printf("rd.aio_buf:%s  wr.aio_nbytes:%d\n",rd.aio_buf, wr.aio_nbytes) ;
#endif
    if(wr.aio_buf == NULL)
    {
        perror("aio_buf");
    }
 
    wr.aio_fildes = fd;
 
    wr.aio_lio_opcode = LIO_WRITE;   ///lio操作类型为异步写
 
    //将异步写事件添加到list中
    listio[1] = &wr;
 
    //使用lio_listio发起一系列请求
    ret = lio_listio(LIO_WAIT,listio,MAX_LIST,NULL);
 
    //当异步读写都完成时获取他们的返回值
 
    ret = aio_return(&rd);
    printf("\n读返回值:%d",ret);
 
    ret = aio_return(&wr);
    printf("\n写返回值:%d",ret);
 
 
 
    return 0;
}