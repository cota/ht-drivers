CPU=	L865

include /acc/src/dsc/co/Make.auto

SRCS=	libvmebus.c

CFLAGS=	-Wall -g -I../include

LIBVMEBUS = libvmebus.$(CPU).a

all: $(LIBVMEBUS) $(SRCS:.c=.$(CPU).o)

libvmebus.$(CPU).o: ../include/vmebus.h libvmebus.c libvmebus.h

libvmebus.$(CPU).a: libvmebus.$(CPU).o
	$(AR) rv $(LIBVMEBUS) $^
	$(RANLIB) $(LIBVMEBUS)

libvmebusdoc: ../include/vmebus.h libvmebus.h libvmebus.c
	mkdir -p doc
	doxygen libvmebus.dox

doc: libvmebusdoc

clean:
	$(RM) *.a *.o $(BAKS)
	$(RM) -r doc
