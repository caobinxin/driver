obj-m := virtualchar.o
CONFIG_MODULE_SIG=n
all :
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules  
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean 

test:
	gcc read.c -o test_read
	gcc write.c -o test_write

run_test_r:
	sudo pwd 
	sudo ./test_read /dev/virtualchar
run_test_w:
	sudo pwd 
	sudo ./test_write /dev/virtualchar
