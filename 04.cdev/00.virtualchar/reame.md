### 说明
本案例 仅仅实现了一个基本的字符设备驱动

在编译过程中遇到的问题汇总：


1. 新版本没有ioctl接口
.ioctl = virtualchar_ioctl ==> .compat_ioctl = virtualchar_ioctl

2. 类型不匹配
printk(KERN_INFO"read %d bytes(s) from %d\n", count, p) ; ==> printk(KERN_INFO"read %ld bytes(s) from %ld\n", count, p) ;

3. insmod: can't insert '×.ko': Device or resource busy 主设备号 被占用，这里设置为0，让系统自己分配
#define VIRTUALCHAR_MAJOR 254 ==> #define VIRTUALCHAR_MAJOR 0




#### 总结
1. 查看设备的主设备好的方法
colby@colby-myntai:~/work300GB/cbx-study/linux-kernel/module/04.cdev/00.virtualchar$ cat /proc/devices | grep virtu
242 virtualchar
==> 主设备号为 242

2. 创建 设备节点
mknod /dev/virtualchar c 242 0

3. 先不用管内核签名的事
先切到root用户下。
