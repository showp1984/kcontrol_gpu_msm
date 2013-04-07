KERNEL_BUILD := /home/showp1984/WORK/kernel/3.4.0-modbuild-kernel-aosp/

obj-m += kcontrol_gpu_msm.o

all:
	make -C $(KERNEL_BUILD) M=$(PWD) modules
	$(CROSS_COMPILE)strip --strip-debug kcontrol_gpu_msm.ko

clean:
	make -C $(KERNEL_BUILD) M=$(PWD) clean 2> /dev/null
	rm -f modules.order *~
