#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    int fd ;
    int counter = 0 ;
    int old_counter = 0 ;

    /*打开/dev/second设备文件*/

    fd = open("/dev/second", O_RDONLY) ;
    if( fd != -1)
    {
        while(1){
            read(fd, &counter, sizeof(unsigned int)) ;
            if( counter != old_counter)
            {
                printf("seconds after open /dev/second: %d\n", counter) ;
                old_counter = counter ;
            }
        }
        
    }else
    {
        printf("Device open failure\n") ;
    }
    
    return 0;
}

/*测试说明
    sudo dmesg -c
        second_major = 242
    
    */