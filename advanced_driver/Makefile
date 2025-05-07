# obj-m += advanced_driver.o

# all:
# 	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
# clean:
# 	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
# Kernel module name
obj-m += advanced_driver.o

# Kernel module build directory
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Default target
all: advanced_driver trigger_ioctl

# Build the kernel module
advanced_driver:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Build the user-space helper
trigger_ioctl: trigger_ioctl.c
	gcc -Wall -o trigger_ioctl trigger_ioctl.c

# Clean up both kernel and user-space files
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) trigger_ioctl
