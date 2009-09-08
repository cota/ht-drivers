#================================================================
# Makefile to produce standard CTR-P test program
#================================================================

CPU=ppc4

include /acc/dsc/src/co/Make.auto

DDIR = xmemn

ACCS=oplhc

CFLAGS= -g -Wall -I. -I../driver -I../../include

LDLIBS = -ltgm -lerr -lnc

ALL  = xmemtest.$(CPU) xmemtest.$(CPU).o

SRCS = xmemtest.c XmemCmds.c XmemOpen.c DoCmd.c GetAtoms.c Cmds.c \
       plx9656.c

HDRS = Cmds.h

TEST = xmemtest.c XmemCmds.c XmemOpen.c DoCmd.c GetAtoms.c Cmds.c plx9656.c

all: $(ALL)

clean:
	$(RM) $(ALL) $(BAKS)

# Run on Workstation only

xmemtest.$(CPU).o: $(TEST) $(HDRS)

install: xmemtest.$(CPU)
	@for f in $(ACCS); do \
	    dsc_install xmemtest.$(CPU) /acc/dsc/$$f/$(CPU)/$(BSP)/$(DDIR); \
	done;

