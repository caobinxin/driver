if [ "$1"="usb" ];then
	if [ "$PRODUCT"="82d/100/0" ];then
		if [ "$ACTION"="add" ];then
			/sbin/modprobe visor
		else
			/sbin/rmmod visor
		fi
	fi
fi