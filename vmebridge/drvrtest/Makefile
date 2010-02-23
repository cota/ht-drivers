#
# Makefile for test drivers.
#
include ../common.mk

all: vmedriver vmeheaders mymodules

.PHONY : all vmedriver vmeheaders mymodules clean help

vmedriver:
	cd ../$(DRVDIR) && $(MAKE)
	cp ../$(DRVDIR)/$(CPU)/$(KVER)/Module.symvers .

vmeheaders:
	$(MAKE) -C ../$(INCDIR)

mymodules:
	$(MAKE) -C $(KERN_DIR) M=`pwd`

help:
	$(MAKE) -C $(KERN_DIR) M=`pwd` help

clean:
	$(MAKE) -C $(KERN_DIR) M=`pwd` clean
