AR	= ar
ARCH	= arm
CC	= arm-none-linux-gnueabi-gcc
DEBFLAGS = -O2
obj-m	:= smodule.o smodule_dep.o
KERNELDIR ?= /root/fgj/linux-4.5.2
PWD       := $(shell pwd)
modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/../include modules
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
