#================================================================
# Makefile to produce mtting library
#================================================================

include /ps/dsc/src/co/Make.auto

CFLAGS= -g -Wall -I. -I/tmp -I/ps/local/$(CPU)/include -I/dsrc/drivers/mtt/driver -I/dsrc/drivers/mtt/lib/xmem

SRCS=libmtt.c libmtt.h

INSTFILES=libmtt.$(CPU).a libmtt.h

ACCS=tst

SHELL = /bin/csh

all:$(INSTFILES)

libmtt.$(CPU).o: libmtt.c libmtt.h /tmp/mttregs.h

/tmp/mttregs.h:
	../assembler/gtm TgmLHC

libmtt.$(CPU).a: libmtt.$(CPU).o
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	rm -f /tmp/mttregs.h
	rm -f libmtt.$(CPU).o
	rm -f libmtt.$(CPU).a

install: $(INSTFILES)
	rm -f /ps/local/$(CPU)/mtt/libmtt.a
	cp libmtt.$(CPU).a /ps/local/$(CPU)/mtt/libmtt.a
	rm -f /ps/local/$(CPU)/mtt/libmtt.h
	cp libmtt.h /ps/local/$(CPU)/mtt/libmtt.h
