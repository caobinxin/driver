#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define MAP_SIZE 4096
#define USTR_DEF "aaaa changed from the User Space"

int main(int argc, char const *argv[])
{
    int fd , ret, count = 0;
    char *pdata;
    char r_buf[MAP_SIZE] ;
    char w_buf[MAP_SIZE] = USTR_DEF;

    if(argc <= 1)
    {
        printf("Usage: main devfile userstring\n") ;
        return 0 ;
    }

    fd = open(argv[1], O_RDWR | O_CREAT,0777) ;
    if(fd >= 0)
    {
        while(1){

            if(w_buf[0] > 100){
                w_buf[0] = 0 ;
            }
            w_buf[0] = w_buf[0] + 1 ;
            w_buf[1] = w_buf[0] ;
            w_buf[2] = w_buf[0] ;
            w_buf[3] = w_buf[0] ;

            ret = write( fd, w_buf, strlen(w_buf));
            if(ret < 0)
            {
                printf("write fail ret = %d\n", ret) ;
            }else
            {
                printf("user write: success!\n");
            }
            sleep(1) ;
            if(count > 100)
            {
                break ;
            }
            count++ ;
        }
        close(fd) ;
        
    }
    return 0;
}


