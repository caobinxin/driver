#define _GNU_SOURCE 
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <libaio.h>

#define BUF_SIZE 4096

int main(int argc, char const *argv[])
{
    io_context_t ctx = 0 ;
    struct iocb cb ;
    struct iocb *cbs[1] ;
    unsigned char *buf ;
    struct io_event events[1] ;

    int ret ;
    int fd ;
    
    if(argc < 2){
        printf("the command format: aior [FILE]\n") ;
        exit(1) ;
    }

    fd = open(argv[1], O_RDWR | O_DIRECT) ;
    if(fd < 0){
        perror("open error") ;
        goto err ;
    }

    //allocate aligned memory
    ret = posix_memalign((void **)&buf, 512, (BUF_SIZE + 1)) ;
    if(ret < 0){
        perror("posix_memalign failed") ;
        goto err1 ;
    }
    memset(buf, 0, BUF_SIZE + 1) ;

    ret = io_setup(128, &ctx) ;
    if(ret < 0){
        printf("io_setup error:%s", strerror(-ret)) ;
        goto err2 ;
    }

    //setup io control block
    io_prep_pread(&cb, fd, buf, BUF_SIZE, 0) ;

    cbs[0] = &cb ;
    ret = io_submit(ctx, 1, cbs) ;
    if(ret != 1){
        if(ret < 0){
            printf("io_submit error:%s", strerror(-ret)) ;
        }else{
            fprintf(stderr, "could not submit ios") ;
        }
        goto err3 ;
    }

    //get the reply
    ret = io_getevents(ctx, 1, 1, events, NULL) ;
    if( ret != 1){
        if( ret < 0){
            printf("io_getevents error:%s", strerror(-ret)) ;
        }else{
            fprintf(stderr, "could not get Events") ;
        }
        goto err3 ;
    }

    if(events[0].res2 == 0){
        printf("%s\n", buf) ;
    }else{
        printf("AIO error:%s", strerror(-events[0].res)) ;
        goto err3 ;
    }

    if( (ret = io_destroy(ctx)) < 0){
        printf("io_destroy error:%s", strerror(-ret)) ;
        goto err2 ;
    }

    free(buf) ;
    close(fd) ;
    return 0;

err3:
    if( (ret = io_destroy(ctx)) < 0){
        printf("io_destroy error:%s", strerror(-ret)) ;
    }
err2:
    free(buf) ;
err1:
    close(fd) ;
err:
    return -1 ;
}


/**
 * 编译的时候发现找不到　libaio.h
 * 安装：　sudo apt-get install libaio-dev
 * 
 * 编译命令：gcc demo.c -laio
 * 
 * 运行：./a.out test.txt
*/