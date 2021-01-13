ifeq ($(KERNELRELEASE),)

PWD:= $(shell pwd)
ARCH=arm
CROSS_COMPILE=arm-none-linux-gnueabi-
KERNELDIR=/root/fgj/linux-4.5.2


lkm.ko:    devicecreatetest.c
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm devicecreatetest.ko *.o *.cmd

else

obj-m += devicecreatetest.o

endif
