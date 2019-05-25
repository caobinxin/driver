char buf ;

fd = open("/dev/ttyS1", O_RDWR) ;

...

res = read(fd, &buf, 1) ;//当串口上有输入时才返回
if(res == 1){
    printf("%c\n", buf) ;
}