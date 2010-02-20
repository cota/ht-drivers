include ../common.mk
include /acc/src/dsc/co/Make.auto


all: clean
	ln -s ../$(DRVDIR)/vmebus.h vmebus.h
	ln -s ../$(LIBDIR)/libvmebus.h libvmebus.h

clean:
	rm -f vmebus.h libvmebus.h
	$(RM) $(BAKS)
