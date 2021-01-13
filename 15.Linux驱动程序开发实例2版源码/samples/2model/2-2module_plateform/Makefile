ifeq ($(KERNELRELEASE),)

PWD:= $(shell pwd)
ARCH=arm
CROSS_COMPILE=arm-none-linux-gnueabi-
KERNELDIR=/root/fgj/linux-4.5.2


lkm.ko:    plateformtest.c
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm plateformtest.ko *.o *.cmd

else

obj-m += plateformtest.o

endif
