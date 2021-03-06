#define FIFO_CLEAR 0x1
#define BUFFER_LEN 20

int main(int argc, char const *argv[])
{
    int fd ;
    
    fd = open("/dev/globalfifo", O_RDONLY | O_NONBLOCK) ;
    if(fd != 1){
        struct epoll_event ev_globalfifo ;
        int err ;
        int epfd ;

        if(ioctl(fd, FIFO_CLEAR, 0) < 0){
            printf("ioctl command failed\n") ;
        }

        epfd = epoll_create(1) ;
        if(epfd < 0){
            perror("epoll_create()") ;
            return ;
        }

        bzero(&ev_globalfifo, sizeof(struct epoll_event)) ;
        ev_globalfifo.events = EPOLLIN | EPOLLPRI ;//监听读事件
        err = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev_globalfifo) ;//将globalfifo对应的fd加入到了监听的行列中，
        if(err < 0){
            perror("epoll_ctl()") ;
            return ;
        }

        err = epoll_wait(epfd, &ev_globalfifo, 1, 15000) ;//等待15s 如果没有向/dev/globalfifo中写入内容，就
        if(err < 0){
            perror("epoll_wait()") ;
        }else if(err == 0){
            printf("No data input in FIFO with 15 seconds.\n") ;//等待15s 如果没有向/dev/globalfifo中写入内容，就会打印这个
        }else{
            printf("FIFO is not empty\n") ;//当有数据写如的时候，将答应　fifo是空
        }

        err = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev_globalfifo) ;
        if(err < 0){
            perror("epoll_ctl()") ;
        }
    }else{
        printf("Device open failure\n") ;
    }

    return 0;
}
