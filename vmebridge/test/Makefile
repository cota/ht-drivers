TARGET_CPU = L865
include /acc/src/dsc/co/Make.auto
include ../../makefiles/rules.mk

SRCS =	create_window.c destroy_window.c setflag.c \
	test_mapping.c testctr.c testctr2.c testctr_ces.c \
	testvd80.c vd80spd.c vd80spd-dma.c mm6390-dma.c \
	testvd802.c hsm-dma.c berrtest.c doublemap.c

CFLAGS		= -Wall -DDEBUG -D_GNU_SOURCE -g -I. -I../include
LIBVMEBUS	= ../object_vmebus/libvmebus.$(CPU).a
LIBVD80		= libvd80.$(CPU).a
LDLIBS		= -lrt

_build: $(SRCS:.c=.$(CPU))

.PHONY: all sources doc clean libvd80doc

sources: $(SRCS) $(SCRIPTS)

testctr_ces.$(CPU): testctr_ces.c $(LIBVMEBUS)
testctr2.$(CPU): testctr2.c $(LIBVMEBUS)

testvd80.$(CPU): testvd80.c $(LIBVD80) $(LIBVMEBUS)
testvd802.$(CPU): testvd802.c $(LIBVMEBUS)
vd80spd.$(CPU): vd80spd.c $(LIBVD80) $(LIBVMEBUS)
vd80spd-dma.$(CPU): vd80spd-dma.c $(LIBVD80) $(LIBVMEBUS)

libvd80.$(CPU).o: libvd80.c libvd80.h

libvd80.$(CPU).a: libvd80.$(CPU).o
	$(AR) rv $(LIBVD80) $^
	$(RANLIB) $(LIBVD80)

libvd80doc: libvd80.c libvd80.h vd80.h
	mkdir -p doc
	doxygen libvd80.dox

mm6390-dma.$(CPU): mm6390-dma.c $(LIBVMEBUS)
hsm-dma.$(CPU): hsm-dma.c $(LIBVMEBUS)
berrtest.$(CPU): berrtest.c $(LIBVMEBUS)
doublemap.$(CPU): doublemap.c $(LIBVMEBUS)

doc: libvd80doc

clean:
	$(RM) *.$(CPU) *.o *.$(CPU).a $(BAKS)
	$(RM) -r doc
