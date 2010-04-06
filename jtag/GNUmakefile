#================================================================
# Makefile to produce standard CTX1 test applications
#================================================================

CPU=L865

include /ps/dsc/src/co/Make.auto

ACCS=tst
CFLAGS = -g -I.
LDLIBS = -lvmebus
ALL  = jtag.$(CPU).o jtag.$(CPU)
SRCS = jtag.c lenval.c micro.c ports.c smemio.c
HDRS = lenval.h micro.h ports.h

all: $(ALL)

# Run on Workstation only

jtag.$(CPU).o: $(SRCS) $(HDRS)

jtag.$(CPU): jtag.$(CPU).o

clean:
	rm -rf *.$(CPU).o *.$(CPU)

