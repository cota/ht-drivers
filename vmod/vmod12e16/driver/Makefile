obj-m += vmod12e16.o

CPU=L865
KVER=2.6.29.4-rt15
DOXY_INSTPATH = /acc/doc/html/private/coht/doxy/vmod12e16/driver

EXTRA_CFLAGS=-g -Wall

modules:
	make -C /acc/sys/$(CPU)/usr/src/kernels/$(KVER)/ M=$(PWD) modules

clean: doxy_clean
	make -C /acc/sys/$(CPU)/usr/src/kernels/$(KVER)/ M=$(PWD) clean

doxy_clean:
	rm -rf doxygen_output

doxy:
	../../../doxy/doxy.sh -n"VMOD 12e16 driver"  . ../include

doxy_install: doxy
	mkdir -p $(DOXY_INSTPATH)
	cp -r doxygen_output/html/* $(DOXY_INSTPATH)/
