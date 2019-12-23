ccflags-y := -std=gnu99 -Wno-declaration-after-statement

# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
	obj-m := translate.o
	translate-objs := module.o caesar.o device.o string.o
    
# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else

	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD		:= $(shell pwd)
    
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
    
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

endif
ifeq (.depend,$(wildcard .depend))
include .depend
    
endif