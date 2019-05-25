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
  * 用户可以使用　aio_suspend()函数来阻塞调用进程，直到异步请求完成为止。调用者提供了一个aiocb引用列表，其中任何一个完成
  * 都会导致aio_suspend()返回。
  * 
 */

#define BUFFER_SIZE 1024
 
int MAX_LIST = 1;
 
int main(int argc,char **argv)
{
    //aio操作所需结构体
    struct aiocb rd;
 
    int fd,ret,couter;
 
    //cblist链表
    struct aiocb *aiocb_list[MAX_LIST];
 
 
 
    fd = open("test.txt",O_RDONLY);
    if(fd < 0)
    {
        perror("test.txt");
    }
 
 
 
    //将rd结构体清空
    bzero(&rd,sizeof(rd));
 
 
    //为rd.aio_buf分配空间
    rd.aio_buf = malloc(BUFFER_SIZE + 1);
 
    //填充rd结构体
    rd.aio_fildes = fd;
    rd.aio_nbytes =  BUFFER_SIZE;
    rd.aio_offset = 0;
 
    //将读fd的事件注册
    aiocb_list[0] = &rd;
 
    //进行异步读操作
    ret = aio_read(&rd);
    if(ret < 0)
    {
        perror("aio_read");
        exit(1);
    }

#if 0
 //  循环等待异步读操作结束
    couter = 0;
    while(aio_error(&rd) == EINPROGRESS)
    {
        printf("第%d次\n",++couter);
    }
 #else
     //阻塞等待异步读事件完成
    printf("我要开始等待异步读事件完成\n");
    ret = aio_suspend(aiocb_list,MAX_LIST,NULL);
 #endif

    printf("查看返回的结果\n") ;
    //获取异步读返回值
    ret = aio_return(&rd);
 
    printf("\n\n返回值为:%d\n",ret);
 
 
    return 0;
}