#================================================================
# Makefile to produce MTT Multitasking CTG test program
#================================================================

include /acc/dsc/src/co/Make.auto

DRV = ../driver

CFLAGS = -g -Wall -fwritable-strings -I. -I$(DRV) -I$(PS) \
	 -I/acc/local/$(CPU)/include -I./jtag -I../lib -I/dsrc/drivers/ctr/src/driver \
	-I../../include -I../driver
LDLIBS = -ltgm -ldscrt -lerr -lerr_dummy -llynx -lnetinet -lm -ltgv -lgm
ALL  = mtttest.$(CPU)
SRCS = mtttest.c MttCmds.c MttOpen.c DoCmd.c GetAtoms.c Cmds.c
HDRS = Cmds.h $(DRV)/mttdrvr.h $(DRV)/mtt_priv.h

all: $(ALL)

clean:
	$(RM) $(ALL) $(BAKS)

# Run on Workstation only

mtttest.$(CPU).o: $(SRCS) $(HDRS)

mtttest.$(CPU): mtttest.$(CPU).o

install: mtttest.$(CPU)
	dsc_install mtttest.$(CPU) /acc/dsc/lab/$(CPU)/ces/mtt
	dsc_install Mtt.xsvf /acc/dsc/lab/$(CPU)/ces/mtt
	dsc_install mtttest.$(CPU) /acc/dsc/oper/$(CPU)/ces/mtt
	dsc_install Mtt.xsvf /acc/dsc/oper/$(CPU)/ces/mtt
	dsc_install mtttest.$(CPU) /acc/dsc/oplhc/$(CPU)/ces/mtt
	dsc_install Mtt.xsvf /acc/dsc/oplhc/$(CPU)/ces/mtt
