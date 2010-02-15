#================================================================
# Makefile to produce standard CTX1 test applications
#================================================================

include /acc/dsc/src/co/Make.auto

ACCS=tst
CFLAGS = -g -I. -I$(ENV_PREFIX)/usr/include/headers_ppc \
	 -I/acc/local/ppc/include -I/acc/local/ppc/include/ces \
	 -I/acc/sys/ppc/usr/include/headers_ppc -I/acc/sys/ppc/sys/include/kernel \
	 -I/acc/sys/ppc/sys/dheaders \
	 -I/acc/sys/ppc/sys/include/family/$(CPU) -Wall \
	 -DCOMPILE_TIME=$(COMPILE_TIME)
LDLIBS = -lvme
ALL  = jtag.$(CPU)
SRCS = jtag.c lenval.c micro.c ports.c smemio.c
HDRS = lenval.h micro.h ports.h

all: $(ALL)

clean:
	$(RM) $(ALL) $(BAKS)

# Run on Workstation only

jtag.$(CPU).o: $(SRCS) $(HDRS)

jtag.$(CPU): jtag.$(CPU).o

install: jtag.$(CPU)
	dsc_install jtag.ppc /dsc/tst/ppc/ces/ctx1
