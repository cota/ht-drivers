DRVR=vmebus ctrirq
ACCS=lab
CPU=L865
KERNEL=2.6.24.7-rt27
KERN_DIR="/acc/sys/$(CPU)/usr/src/kernels/$(KERNEL)"

include /acc/src/dsc/co/Make.auto

DRVDIR=$(shell pwd)/driver
DRVTESTDIR=$(shell pwd)/drvrtest

all: drvr includes lib drvrtest test

drvr:
	make -C $(KERN_DIR) M=$(DRVDIR)

# We need to copy the vmebus driver symbols file to use its exported symbols
# because KBUILD_EXTRA_SYMBOLS is not there yet to use in 2.6.24.7-rt21
drvrtest: drvr includes
	cp $(DRVDIR)/Module.symvers $(DRVTESTDIR)
	make -C $(KERN_DIR) M=$(DRVTESTDIR)

lib:
	make -C lib

test: drvr includes lib
	make -C test

includes:
	make -C include

clean:
	make -C $(KERN_DIR) M=$(DRVDIR) clean
	make -C drvr clean
	make -C $(KERN_DIR) M=$(DRVTESTDIR) clean
	make -C drvrtest clean
	make -C lib clean
	make -C test clean
	make -C include clean
	$(RM) $(BAKS)

.PHONY: drvr drvrtest includes lib test clean

install:: driver/vmebus.ko driver/vmebus.h lib/libvmebus.h
	for a in $(ACCS);do \
	    if [ -w /acc/dsc/$$a/$(CPU)/$(KERNEL)/$(DRVR) ]; then \
		echo Installing TSI148 VME bridge driver in /acc/dsc/$$a/$(CPU)/$(KERNEL)/$(DRVR); \
		dsc_install driver/vmebus.ko /acc/dsc/$$a/$(CPU)/$(KERNEL)/$(DRVR); \
#		make -C apps INSTDIR=/acc/dsc/$$a/$(CPU)/$(KERNEL)/$(DRVR) install; \
	    fi; \
	done
	dsc_install driver/vmebus.h /acc/local/$(CPU)/include
	dsc_install lib/libvmebus.h /acc/local/$(CPU)/include

MY= \
    -I/acc/sys/$(CPU)/usr/src/kernels/$(KERNEL)/include  \
    -I/acc/sys/$(CPU)/usr/src/kernels/$(KERNEL)/include/asm-x86/mach-generic \
    -I/acc/sys/$(CPU)/usr/src/kernels/$(KERNEL)/include/asm-x86/mach-default \
    -D__KERNEL__ -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1

ANSI.h: driver/tsi148.c driver/vme_irq.c driver/vme_window.c driver/vme_dma.c \
	driver/vme_misc.c driver/vme_cesif.c driver/vme_bridge.c
	rm -f $@
	for f in $^; do cproto -es $(MY) -o /tmp/ANSI.h $$f; cat /tmp/ANSI.h >>$@; done
	rm -f /tmp/ANSI.h

