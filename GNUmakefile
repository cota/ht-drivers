#================================================================
# Makefile to produce standard CTR-P test program
#================================================================

CPU=ppc4

include /acc/dsc/src/co/Make.auto

DDIR = xmem

ACCS=oplhc

CFLAGS= -g -Wall -I. -I/dsrc/drivers/xmem/src/driver -I../../include

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
	    rm -f /acc/dsc/$$f/$(CPU)/$(BSP)/$(DDIR)/Xmem.segs; \
	    rm -f /acc/dsc/$$f/data/$(DDIR)/Xmem.segs; \
	    cp Xmem.segs /acc/dsc/$$f/$(CPU)/$(BSP)/$(DDIR); \
	    chmod 444 /acc/dsc/$$f/$(CPU)/$(BSP)/$(DDIR)/Xmem.segs; \
	    cp Xmem.segs /acc/dsc/$$f/data/$(DDIR); \
	    chmod 664 /acc/dsc/$$f/data/$(DDIR)/Xmem.segs; \
	done;

