#define _GNU_SOURCE
#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <errno.h>
#include <libaio.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#define TEST_FILE   "aio_test_file"
#define TEST_FILE_SIZE  (127 * 1024)
#define NUM_EVENTS  128
#define ALIGN_SIZE  512
#define RD_WR_SIZE  1024

struct custom_iocb
{
    struct iocb iocb;
    int nth_request;
};

void aio_callback(io_context_t ctx, struct iocb *iocb, long res, long res2)
{
    struct custom_iocb *iocbp = (struct custom_iocb *)iocb;
    printf("nth_request: %d, request_type: %s, offset: %lld, length: %lu, res: %ld, res2: %ld\n", 
            iocbp->nth_request, (iocb->aio_lio_opcode == IO_CMD_PREAD) ? "READ" : "WRITE",
            iocb->u.c.offset, iocb->u.c.nbytes, res, res2);
}

int main(int argc, char *argv[])
{
    int efd, fd, epfd;
    io_context_t ctx;
    struct timespec tms;
    struct io_event events[NUM_EVENTS];
    struct custom_iocb iocbs[NUM_EVENTS];
    struct iocb *iocbps[NUM_EVENTS];
    struct custom_iocb *iocbp;
    int i, j, r;
    void *buf;
    struct epoll_event epevent;

    /**
     * eventfd()作用是内核用来通知应用程序发生的事件的数量，从而使应用程序
     * 不用频繁地去轮询内核是否有事件发生，而是有内核将发生事件的数量
     * 写入到该fd，应用程序发现fd可读后，从fd读取该数值，并马
     * 上去内核读取。
    */

    efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (efd == -1) {
        perror("eventfd");
        return 2;
    }

    fd = open(TEST_FILE, O_RDWR | O_CREAT | O_DIRECT, 0644);
    if (fd == -1) {
        perror("open");
        return 3;
    }
    /**
     * ftruncate()会将参数fd指定的文件大小改为参数length指定的大小。
     * 参数fd为已打开的文件描述词，而且必须是以写入模式打开的文件。
     * 如果原来的文件件大小比参数length大，则超过的部分会被删去
    */
    ftruncate(fd, TEST_FILE_SIZE);
    
    //1. 建立io任务
    ctx = 0;
    if (io_setup(8192, &ctx)) {
        perror("io_setup");
        return 4;
    }

    //给buf分配空间　并　对齐  这里注意读写的buf都必须是按扇区对齐的，可以用posix_memalign来分配。
    if (posix_memalign(&buf, ALIGN_SIZE, RD_WR_SIZE)) {
        perror("posix_memalign");
        return 5;
    }
    printf("buf: %p\n", buf);

    for (i = 0, iocbp = iocbs; i < NUM_EVENTS; ++i, ++iocbp) {
        iocbps[i] = &iocbp->iocb;
        io_prep_pread(&iocbp->iocb, fd, buf, RD_WR_SIZE, i * RD_WR_SIZE);//提交任务之前必须先填充iocb结构体
        io_set_eventfd(&iocbp->iocb, efd);
        io_set_callback(&iocbp->iocb, aio_callback);
        iocbp->nth_request = i + 1;
    }

    //2. 提交io任务
    if (io_submit(ctx, NUM_EVENTS, iocbps) != NUM_EVENTS) {
        perror("io_submit");
        return 6;
    }

    epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        return 7;
    }

    epevent.events = EPOLLIN | EPOLLET;
    epevent.data.ptr = NULL;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &epevent)) {
        perror("epoll_ctl");
        return 8;
    }

    i = 0;
    while (i < NUM_EVENTS) {
        uint64_t finished_aio;

        if (epoll_wait(epfd, &epevent, 1, -1) != 1) {
            perror("epoll_wait");
            return 9;
        }

        if (read(efd, &finished_aio, sizeof(finished_aio)) != sizeof(finished_aio)) {
            perror("read");
            return 10;
        }

        printf("finished io number: %"PRIu64"\n", finished_aio);
    
        while (finished_aio > 0) {
            tms.tv_sec = 0;
            tms.tv_nsec = 0;
            //3.获取完成的IO
            r = io_getevents(ctx, 1, NUM_EVENTS, events, &tms);
            if (r > 0) {
                for (j = 0; j < r; ++j) {
                    ((io_callback_t)(events[j].data))(ctx, events[j].obj, events[j].res, events[j].res2);
                }
                i += r;
                finished_aio -= r;
            }
        }
    }
    
    close(epfd);
    free(buf);
    io_destroy(ctx);
    close(fd);
    close(efd);
    remove(TEST_FILE);

    return 0;
}

/**
 * 
 * 说明：
1. 运行通过
2. struct io_event中的res字段表示读到的字节数或者一个负数错误码。在后一种情况下，-res表示对应的
   errno。res2字段为0表示成功，否则失败
3. iocb在aio请求执行过程中必须是valid的
4. 在上面的程序中，通过扩展iocb结构来保存额外的信息(nth_request)，并使用iocb.data
   来保存回调函数的地址。如果回调函数是固定的，那么也可以使用iocb.data来保存额外信息。
*/


/*
有了eventfd，就可以很好地将libaio和epoll事件循环结合起来：

1. 创建一个eventfd

efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

2. 将eventfd设置到iocb中

io_set_eventfd(iocb, efd);

3. 交接AIO请求

io_submit(ctx, NUM_EVENTS, iocb);

4. 创建一个epollfd，并将eventfd加到epoll中

epfd = epoll_create(1);

epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &epevent);

epoll_wait(epfd, &epevent, 1, -1);

5. 当eventfd可读时，从eventfd读出完成IO请求的数量，并调用io_getevents获取这些IO

read(efd, &finished_aio, sizeof(finished_aio);

r = io_getevents(ctx, 1, NUM_EVENTS, events, &tms);

*/