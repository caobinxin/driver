ifeq ($(KERNELRELEASE),)

#KERNELDIR ?= /home/linux/linux-3.14-fs4412/
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) 
	rm -rf *.o *~ core .depend .*.cmd  *.mod.c .tmp_versions Module* modules*

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module* modules*

.PHONY: modules modules_install clean

else
    obj-m := pcie_net_dri.o
endif

