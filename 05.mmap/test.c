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
    int fd ;
    char *pdata;

    if(argc <= 1)
    {
        printf("Usage: main devfile userstring\n") ;
        return 0 ;
    }

    fd = open(argv[1], O_RDWR | O_NDELAY) ;
    if(fd >= 0)
    {
        //printf("测试这个函数　strtoul(argv[2], 0, 16) = %x\n", strtoul(argv[2], 0, 16)) ;

        pdata = (char *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0) ;
        printf("UserAddr=%p, data from kernel: %s\n", pdata, pdata) ;
        
        printf("Writing a string to the kernel space ... ") ;
        strcpy(pdata, argv[2]) ;
        printf("Done\n") ;

        munmap(pdata, MAP_SIZE) ;
        close(fd) ;

    }
    return 0;
}


/**
 * 
 * 测试流程：
 *          sudo dmesg -c
 * 
 *              [15626.622656] 内核空间中高端内存中页的物理地址　kpa = 0xA30EB000, kernel string = Hello world from kernel virtual space
                [15626.622658] major = 242, minor = 0
            
            gcc test.c -o test

            sudo mknod /dev/demo_map c 242 0

            sudo ./test /dev/demo_map myntai智能
*/