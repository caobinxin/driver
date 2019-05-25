obj-m := add.o

all :
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules  
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean 

load:
	sudo insmod add.ko

play:
	sudo cat /proc/kallsyms | less | grep add_integar