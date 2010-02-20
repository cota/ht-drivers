include common.mk

ACCS = lab

all: drvr includes lib drvrtest test

drvr:
	$(MAKE) -C $(DRVDIR)

drvrtest: drvr includes
	$(MAKE) -C $(DRVTESTDIR)

lib:
	$(MAKE) -C $(LIBDIR)

test: drvr includes lib
	$(MAKE) -C $(TESTDIR)

includes:
	$(MAKE) -C $(INCDIR)

clean:
	$(MAKE) -C $(DRVDIR) clean
	$(MAKE) -C $(DRVTESTDIR) clean
	$(MAKE) -C $(LIBDIR) clean
	$(MAKE) -C $(TESTDIR) clean
	$(MAKE) -C $(INCDIR) clean
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
