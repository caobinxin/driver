热插拔机制：


开启热插拔机制：
    内核得支持　　
        向/proc/sys/kernel/hotplug 文件中写入　
        su root
        echo "/sbin/cbx_hotplug" > /proc/sys/kernel/hotplug

清理log
colby@colby-myntai:~$ su
root@colby-myntai:/proc/sys/kernel# cat /dev/null > /var/log/syslog


sudo insmod kernel_supply.ko

然后　向对应的　属性文件写入　数字，内核态　　根据不同的数字　调用不同的kobject_uevent函数。

我们这里的　/sbin/cbx_hotplug完全可以是一个脚本程序，实际上　/sbin/hotplug的用法就是通过脚本　来加载卸载模块的。
如果系统中有 udevd守护进程，那么它应该一直在监听　kobject_uevent　通过　netlink广播出去的uevent数据包。

内核空间通过　kobject_uevent这个函数，实现了　将内核中发生的一些事件通知到了用户空间。


hotplug.sh 是一个热插拔的脚本　实例代码

