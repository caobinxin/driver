obj-m := virtualchar.o
CONFIG_MODULE_SIG=n
all :
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules  
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean 
