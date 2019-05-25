#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#define MAX_LEN 100

void input_handler(int num)
{
    char data[MAX_LEN] ;
    int len ;

    /*读取并输出STDIN_FILENO　上的输入*/
    len = read(STDIN_FILENO, &data, MAX_LEN) ;
    data[len] = 0 ;
    printf("input available: %s\n", data) ;
}

int main(int argc, char const *argv[])
{
    int oflags ;

    /*启动信号驱动机制*/

    signal(SIGIO, input_handler) ;

    fcntl(STDIN_FILENO, __F_SETOWN, getpid()) ;//设置本进程为STDIN_FILENO文件的拥有者，如果没有这一步，内核不会知道应该将信号发给那个进程

    oflags = fcntl(STDIN_FILENO, F_GETFL) ;
    fcntl(STDIN_FILENO, F_SETFL, oflags | FASYNC) ;//为了启用异步通知机制，还需要对设备设置 FASYNC标志

    while(1);
    
    return 0;
}
