#================================================================
# Makefile to produce Xmem Daemon  program
#================================================================

CPU=ppc4

include /acc/dsc/src/co/Make.auto

DDIR=ctg

CFLAGS= -g -Wall -I. -I..

ifeq ($(CPU), L864)
LDLIBS = ../libxmem.$(CPU).a -lrt
endif

ifeq ($(CPU), L865)
LDLIBS = ../libxmem.$(CPU).a -lrt
endif

ifeq ($(CPU), ppc4)
LDLIBS = ../libxmem.$(CPU).a
endif

ALL  = XmemDaemon.$(CPU) XmemDaemon.$(CPU).o

SRCS = XmemDaemon.c

HDRS = XmemDaemon.h ../libxmem.h

DAEMON = XmemDaemon.c

all: $(ALL)

clean:
	$(RM) $(ALL) $(BAKS)

# Run on Workstation only

XmemDaemon.$(CPU).o: $(DAEMON) $(HDRS)

install:   $(ALL)
	dsc_install XmemDaemon.$(CPU) /acc/dsc/lab/$(CPU)/xmem
	dsc_install XmemDaemon.$(CPU) /acc/dsc/oper/$(CPU)/xmem
	dsc_install XmemDaemon.$(CPU) /acc/dsc/oplhc/$(CPU)/xmem

