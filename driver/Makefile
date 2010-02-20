#
# Makefile for the VME bridge device driver.
#
include ../common.mk

all:
	$(MAKE) -C $(KERN_DIR) M=`pwd`

.PHONY: all clean help

clean:
	$(MAKE) -C $(KERN_DIR) M=`pwd` clean

help:
	$(MAKE) -C $(KERN_DIR) M=`pwd` help
