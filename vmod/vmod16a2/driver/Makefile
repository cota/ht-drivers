CPU=L865
KVER=2.6.29.4-rt15
KERNELPATH=/acc/sys/$(CPU)/usr/src/kernels/$(KVER)/

# call from kernel build system
obj-m += vmod16a2.o
obj-m += carrier.o

all: modules tests
modules:
	make -C $(KERNELPATH) M=$(PWD) modules
clean:
	rm -f tst16a2 tstlib16a2
	make -C $(KERNELPATH) M=$(PWD) clean

# test programs
tests:tst16a2 tstlib16a2
tst16a2 tstlib16a2: libvmod16a2.h
tstlib16a2: libvmod16a2.a 

vmod16a2.o libvmod16a2.o: libvmod16a2.h vmod16a2.h
libvmod16a2.a: libvmod16a2.o 
	ar r $@ $^

