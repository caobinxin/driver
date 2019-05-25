
CURRENT_MODULE_NAME=virtualio

obj-m := $(CURRENT_MODULE_NAME).o
CONFIG_MODULE_SIG=n

UBUNTU_PATH=/lib/modules/$(shell uname -r)/build

ANDROID_3399_PATH=/home/colby/androidos/bottom_layer_work/FireNow-Nougat/kernel

all :
	$(MAKE) -C $(ANDROID_3399_PATH) M=$(PWD) modules  
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean 

push:all
	adb root
	adb remount /
	sleep 1s
	adb push $(CURRENT_MODULE_NAME).ko /system/

test:push
	adb root
	adb shell rmmod $(CURRENT_MODULE_NAME)
	adb shell insmod /system/$(CURRENT_MODULE_NAME).ko
	sleep 1s
	adb shell ./system/bin/$(CURRENT_MODULE_NAME)