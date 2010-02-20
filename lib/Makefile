include ../common.mk
include /acc/src/dsc/co/Make.auto

SRCS=	libvmebus.c

CFLAGS=	-Wall -g -I../$(INCDIR)

LIBVMEBUS = libvmebus.$(CPU).a

all: $(LIBVMEBUS) $(SRCS:.c=.$(CPU).o)

.PHONY: all vmeheaders doc libvmebusdoc clean

vmeheaders:
	$(MAKE) -C ../$(INCDIR)

libvmebus.$(CPU).o: vmeheaders ../$(INCDIR)/vmebus.h libvmebus.c libvmebus.h

libvmebus.$(CPU).a: libvmebus.$(CPU).o
	$(AR) rv $(LIBVMEBUS) $^
	$(RANLIB) $(LIBVMEBUS)

libvmebusdoc: vmeheaders ../$(INCDIR)/vmebus.h libvmebus.h libvmebus.c
	mkdir -p doc
	doxygen libvmebus.dox

doc: libvmebusdoc

clean:
	$(RM) *.a *.o $(BAKS)
	$(RM) -r doc
