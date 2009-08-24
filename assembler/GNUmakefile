#================================================================
# Makefile to produce standard BST test applications
#================================================================

include /acc/dsc/src/co/Make.auto

CFLAGS = -g -Wall -fwritable-strings -I. -I$(FRM) -I$(DRV) -I$(PS) \
	 -I/acc/local/$(CPU)/include

LDLIBS = -ltgv -ltgm -lerr

ALL  = asm.$(CPU) emu.$(CPU) cmt.$(CPU) gtm.$(CPU) nzc.$(CPU) /tmp/mttregs.h
SRCS = asm.c opcodes.c emu.c cmt.c gtm.c nzc.c
HDRS = asm.h

all: $(ALL)

clean:
	$(RM) $(ALL) $(BAKS)

# Run on Workstation only

asm.$(CPU).o: $(SRCS) $(HDRS)

asm.$(CPU): asm.$(CPU).o

emu.$(CPU).o: $(SRCS) $(HDRS)

emu.$(CPU): emu.$(CPU).o

cmt.$(CPU).o: $(SRCS) $(HDRS)

cmt.$(CPU): cmt.$(CPU).o

gtm.$(CPU).o: $(SRCS) $(HDRS)

gtm.$(CPU): gtm.$(CPU).o

nzc.$(CPU).o: $(SRCS) $(HDRS)

nzc.$(CPU): nzc.$(CPU).o

/tmp/mttregs.h:
	rm -f /tmp/mttregs.h
	echo "setenv TGM_NETWORK OPER.LHC; wait_tgm LHC; gtm TgmLHC" | csh -s
	chmod 664 /tmp/mttregs.h

install: $(ALL)
	rm -f /acc/local/ppc4/mtt/asm.h
	cp asm.h /acc/local/ppc4/mtt/asm.h
	dsc_install asm.$(CPU) /acc/dsc/oper/$(CPU)/mtt
	dsc_install emu.$(CPU) /acc/dsc/oper/$(CPU)/mtt
	dsc_install cmt.$(CPU) /acc/dsc/oper/$(CPU)/mtt
	dsc_install nzc.$(CPU) /acc/dsc/oper/$(CPU)/mtt
	dsc_install gtm.$(CPU) /acc/dsc/oper/$(CPU)/mtt
	dsc_install asm.$(CPU) /acc/dsc/lab/$(CPU)/mtt
	dsc_install emu.$(CPU) /acc/dsc/lab/$(CPU)/mtt
	dsc_install cmt.$(CPU) /acc/dsc/lab/$(CPU)/mtt
	dsc_install nzc.$(CPU) /acc/dsc/lab/$(CPU)/mtt
	dsc_install gtm.$(CPU) /acc/dsc/lab/$(CPU)/mtt
	dsc_install asm.$(CPU) /acc/dsc/oplhc/$(CPU)/mtt
	dsc_install emu.$(CPU) /acc/dsc/oplhc/$(CPU)/mtt
	dsc_install cmt.$(CPU) /acc/dsc/oplhc/$(CPU)/mtt
	dsc_install nzc.$(CPU) /acc/dsc/oplhc/$(CPU)/mtt
	dsc_install gtm.$(CPU) /acc/dsc/oplhc/$(CPU)/mtt
