ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m  := home5.o
ccflags-y := -I$(src)/inc
else
# normal makefile
KDIR := /home/parallels/linux-4.4.59

default:
	$(MAKE) -C $(KDIR) M=$$PWD
clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
endif

