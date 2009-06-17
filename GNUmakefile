#================================================================
# Makefile to produce timing library
#================================================================

include /ps/dsc/src/co/Make.auto

CFLAGS= -g -Wall -I. -I/acc/src/dsc/drivers/coht/xmem/driver \
		     -I/ps/local/$(CPU)/include \
		     -I/dsrc/drivers/symp/driver \
		     -I/dsrc/drivers/ctr/src/driver \
		     -I/acc/src/dsc/drivers/coht/include

SRCS=libxmem.c libxmem.h vmic/VmicLib.c shmem/ShmemLib.c network/NetworkLib.c

INSTFILES=libxmem.$(CPU).a libxmem.h

ACCS=tst

all:$(INSTFILES)

libxmem.$(CPU).o: libxmem.c

libxmem.$(CPU).a: libxmem.$(CPU).o
	-$(RM) $@
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	rm -f libxmem.$(CPU).o
	rm -f libxmem.$(CPU).a

install: $(INSTFILES)
	rm -f /ps/local/$(CPU)/xmem/libxmem.a
	cp libxmem.$(CPU).a /ps/local/$(CPU)/xmem/libxmem.a
	rm -f /ps/local/$(CPU)/xmem/libxmem.h
	cp libxmem.h /ps/local/$(CPU)/xmem/libxmem.h
