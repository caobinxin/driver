ifeq ($(KERNELRELEASE),)

PWD:= $(shell pwd)
ARCH=arm
CROSS_COMPILE=arm-none-linux-gnueabi-
KERNELDIR=/root/fgj/linux-4.5.2


lkm.ko:    tasklet.c
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(PWD) modules

else

obj-m += tasklet.o

endif

write: write.c  
	arm-none-linux-gnueabi-gcc $< -o $@  -g  

read: read.c  
	arm-none-linux-gnueabi-gcc $< -o $@  -g  

  
clean:
	rm -rf *.o *.ko *.mod.c .*.cmd *.markers *.order *.symvers .tmp_versions write
