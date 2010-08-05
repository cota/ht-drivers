CPU	:= L865
KVER	:= 2.6.24.7-rt27
KERN_DIR:= "/acc/sys/$(CPU)/usr/src/kernels/$(KVER)"
VME_DIR := "../../vmebridge/driver"
VME_OBJDIR := "../../vmebridge/driver/$(CPU)/$(KVER)"


all: Module.symvers
	$(MAKE) -C $(KERN_DIR) M=`pwd`
.PHONY: all clean help

Module.symvers:
	$(MAKE) -C $(VME_DIR) CPU=$(CPU) KVER=$(KVER)
	cp $(VME_OBJDIR)/Module.symvers .

clean:
	$(MAKE) -C $(KERN_DIR) M=`pwd` clean

help:
	$(MAKE) -C $(KERN_DIR) M=`pwd` help
