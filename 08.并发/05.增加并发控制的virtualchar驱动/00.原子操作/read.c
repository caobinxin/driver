#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define MAP_SIZE 4096
#define USTR_DEF "String changed from the User Space"

int main(int argc, char const *argv[])
{
    int fd , num, count;
    char *pdata;
    char r_buf[MAP_SIZE] ;
    char w_buf[MAP_SIZE] ;

    if(argc <= 1)
    {
        printf("Usage: main devfile userstring\n") ;
        return 0 ;
    }

    
    if(fd >= 0)
    {
        while(1){
            fd = open(argv[1], O_RDWR | O_NDELAY ) ;
            num = read( fd, r_buf, MAP_SIZE);
            printf("user read: %s\n", r_buf) ;

            sleep(2) ;

            if(count > 100)
            {
                break ;
            }
            count++ ;
            close(fd) ;
        }
        
        
    }
    return 0;
}


/**
#### 总结
1. 查看设备的主设备好的方法
colby@colby-myntai:~/work300GB/cbx-study/linux-kernel/module/04.cdev/00.virtualchar$ cat /proc/devices | grep virtu
242 virtualchar
==> 主设备号为 242

2. 创建 设备节点
mknod /dev/virtualchar c 242 0

3. 先不用管内核签名的事
先切到root用户下。

4. 运行 在多个终端窗口运行　make run_test_r　和　make run_test_w 观察　read出的结果有没有乱码，发现没有

5. 通过　 sudo tail -f /var/log/syslog　不断监视内核中驱动的打印信息


*/

/*总结：
    本次调试中发现：
    １　如果每次不重新打开设备节点　重新read，　而是用一个read不断循环，从内核log中我们发现，只有第一次打开设备的时候，第一次read是成功的，其他while中的read都是失败的
        所以这里，不得不　每次关闭重新打开设备
        
    ２　犯了一个比较傻逼的错误，ret = write( fd, w_buf, strlen(w_buf));　写的时候，　write传参应该　传入实际的字符串的长度，而我直接传入了 w_buf的大小了，导致第一次写入成功，再次写入的时候，失败并返回-1
    
*/