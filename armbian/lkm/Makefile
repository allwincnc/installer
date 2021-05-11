MODULE_NAME = arisc_admin
DRV_DIR = /lib/modules/$(shell uname -r)/kernel/drivers/arisc/
BUILD_DIR = /lib/modules/$(shell uname -r)/build
CONF_DIR = /etc/modules-load.d/
CONF_FILE = arisc.conf

obj-m += $(MODULE_NAME).o

all:
	make -C $(BUILD_DIR) M=$(PWD) modules
clean:
	make -C $(BUILD_DIR) M=$(PWD) clean
	-rm -f $(CONF_FILE)
insmod:
	sudo dmesg -C
	-sudo rm -f /dev/$(MODULE_NAME)
	sudo insmod $(MODULE_NAME).ko
	dmesg
rmmod:
	sudo dmesg -C
	sudo rmmod $(MODULE_NAME).ko
	dmesg
install:
	-sudo mkdir $(DRV_DIR)
	echo $(MODULE_NAME) > $(CONF_FILE)
	sudo cp $(CONF_FILE) $(CONF_DIR)$(CONF_FILE)
	sudo cp $(MODULE_NAME).ko $(DRV_DIR)
	sudo depmod
	sudo modprobe $(MODULE_NAME)
remove:
	sudo rmmod $(MODULE_NAME)
	sudo rm -f $(CONF_DIR)$(CONF_FILE)
	sudo rm -f $(DRV_DIR)$(MODULE_NAME).ko
	-sudo rm -rf $(DRV_DIR)
