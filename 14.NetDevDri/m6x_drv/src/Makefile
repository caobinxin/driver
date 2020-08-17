PWD := $(shell pwd)
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

MODULE_NAME = m6xdrv

obj-m     := $(MODULE_NAME).o
$(MODULE_NAME)-objs := pcie.o pps.o uart.o eth.o ioctl.o

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules
	rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.symvers *.order
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
