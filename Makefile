KERNEL_BUILD := /home/showp1984/WORK/kernel/bricked-mako/

obj-m += kcontrol_gpu_msm.o

all:
	make -C $(KERNEL_BUILD) M=$(PWD) modules
	$(KERNEL_CROSS_COMPILE)strip --strip-debug kcontrol_gpu_msm.ko

clean:
	make -C $(KERNEL_BUILD) M=$(PWD) clean 2> /dev/null
	rm -f modules.order *~
