PWD := $(shell pwd)
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

MODULE_NAME = dma_pcie

obj-m     := $(MODULE_NAME).o
$(MODULE_NAME)-objs := dma_test.o

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules
	rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.symvers *.order
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
