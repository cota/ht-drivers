DRVR = vmebus
ACCS = lab
CPU  = L865
KVER = 2.6.24.7-rt27
# KVER=2.6.29.4-rt15
KERN_DIR="/acc/sys/$(CPU)/usr/src/kernels/$(KVER)"

include /acc/src/dsc/co/Make.auto

DRVDIR=$(shell pwd)/driver
DRVTESTDIR=$(shell pwd)/drvrtest

all: drvr includes lib drvrtest test

drvr:
	$(MAKE) -C $(DRVDIR)

drvrtest: drvr includes
	$(MAKE) -C $(DRVTESTDIR)

lib:
	make -C lib

test: drvr includes lib
	make -C test

includes:
	make -C include

clean:
	$(MAKE) -C $(DRVDIR) clean
	$(MAKE) -C $(DRVTESTDIR) clean
	make -C lib clean
	make -C test clean
	make -C include clean
	$(RM) $(BAKS)

.PHONY: drvr drvrtest includes lib test clean

install:: driver/vmebus.ko driver/vmebus.h lib/libvmebus.h
	for a in $(ACCS);do \
	    if [ -w /acc/dsc/$$a/$(CPU)/$(KVER)/$(DRVR) ]; then \
		echo Installing TSI148 VME bridge driver in /acc/dsc/$$a/$(CPU)/$(KVER)/$(DRVR); \
		dsc_install driver/vmebus.ko /acc/dsc/$$a/$(CPU)/$(KVER)/$(DRVR); \
#		make -C apps INSTDIR=/acc/dsc/$$a/$(CPU)/$(KVER)/$(DRVR) install; \
	    fi; \
	done
	dsc_install driver/vmebus.h /acc/local/$(CPU)/include
	dsc_install lib/libvmebus.h /acc/local/$(CPU)/include
