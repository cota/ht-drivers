#================================================================
# Makefile to produce standard CTR-P test program
#================================================================

vpath %.c .
vpath %.h .

CPU=L865

include /ps/dsc/src/co/Make.auto

CFLAGS= -g -Wall -I/ps/local/$(CPU)/include/tgm -I../driver_pci -I../common_pci_vme -I. \
	-I../driver_pci/linux -DCTR_PCI -DPS_VER -DEMULATE_LYNXOS_ON_LINUX

LDLIBS= -ltgm -lvmtg -ltgv -lerr -lerr_dummy -lX11 -lm

ALL  = Launch.$(CPU) \
       CtrReadInfo.$(CPU) \
       CtrWriteInfo.$(CPU) \
       CtrLookat.$(CPU) \
       CtrClock.$(CPU) \
       Ctrv.xsvf \
       Vhdl.versions

SRCS = CtrOpen.c DoCmd.c GetAtoms.c Cmds.c \
       plx9030.c CtrReadInfo.c CtrWriteInfo.c CtrClock.c CtrLookat.c
HDRS = Cmds.h

LOOK = CtrLookat.c DisplayLine.c CtrOpen.c
READ = CtrReadInfo.c CtrOpen.c
WRIT = CtrWriteInfo.c CtrOpen.c
CLOK = CtrClock.c CtrOpen.c
LNCH = Launch.c

all: $(ALL)

clean:
	$(RM) $(ALL) $(BAKS)

# Run on Workstation only

Launch.$(CPU).o: $(LNCH)

CtrReadInfo.$(CPU).o: $(READ) $(HDRS)

CtrWriteInfo.$(CPU).o: $(WRIT) $(HDRS)

CtrClock.$(CPU).o: $(CLOK) $(HDRS)

CtrLookat.$(CPU).o: $(LOOK) $(HDRS)

ctrtest.$(CPU): ctrtest.$(CPU).o

CtrReadInfo.$(CPU): CtrReadInfo.$(CPU).o

CtrWriteInfo.$(CPU): CtrWriteInfo.$(CPU).o

CtrClock.$(CPU): CtrClock.$(CPU).o

CtrLookat.$(CPU): CtrLookat.$(CPU).o

Ctrv.xsvf:

Vhdl.versions:

install: $(ALL)
	dsc_install $(ALL) /acc/dsc/lab/$(CPU)/ctr
	dsc_install $(ALL) /acc/dsc/oper/$(CPU)/ctr
	dsc_install $(ALL) /acc/dsc/oplhc/$(CPU)/ctr
