#================================================================
# Makefile to produce standard CTR-P test program
#================================================================

CPU=ppc4

include /acc/dsc/src/co/Make.auto

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

ALL  = xmemdiag.$(CPU) xmemdiag.$(CPU).o

SRCS = xmemdiag.c XmemCmds.c DoCmd.c GetAtoms.c Cmds.c PrintAtoms.c

HDRS = Cmds.h XmemDiag.h ../libxmem.h

DIAG = xmemdiag.c XmemCmds.c DoCmd.c GetAtoms.c Cmds.c PrintAtoms.c

all: $(ALL)

clean:
	$(RM) $(ALL) $(BAKS)

# Run on Workstation only

xmemdiag.$(CPU).o: $(DIAG) $(HDRS)

install:   $(ALL)
	dsc_install xmemdiag.$(CPU) /acc/dsc/lab/$(CPU)/xmem
	dsc_install xmemdiag.$(CPU) /acc/dsc/oper/$(CPU)/xmem
	dsc_install xmemdiag.$(CPU) /acc/dsc/oplhc/$(CPU)/xmem

