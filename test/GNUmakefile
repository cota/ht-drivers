#================================================================
# Makefile to produce MTT Multitasking CTG test program
#================================================================

include /acc/dsc/src/co/Make.auto

DRV = ../driver

CFLAGS = -g -Wall -fwritable-strings -I. -I$(DRV) -I$(PS) \
	 -I/acc/local/$(CPU)/include -I./jtag -I../lib -I/dsrc/drivers/ctr/src/driver \
	-I../../include -I../driver
LDLIBS = ../lib/libmtt.$(CPU).a -ltgm -ldscrt -lerr -lerr_dummy -llynx -lnetinet -lm -ltgv -lgm
ALL  = mtttest.$(CPU)
SRCS = mtttest.c MttCmds.c MttOpen.c DoCmd.c GetAtoms.c Cmds.c
HDRS = Cmds.h $(DRV)/mttdrvr.h $(DRV)/mtt_priv.h

all: $(ALL)

clean:
	$(RM) $(ALL) $(BAKS)

# Run on Workstation only

mtttest.$(CPU).o: $(SRCS) $(HDRS)

mtttest.$(CPU): mtttest.$(CPU).o

INSTALL_FILES=mtttest.$(CPU) Mtt.xsvf
INSTALL_DIR=mttn
install: $(INSTALL_FILES)
	@for a in $(ACCS); do \
		p=/acc/dsc/$$a/$(CPU)/$(INSTALL_DIR);\
		echo "===============================";\
		echo "Installing in $$p :";\
		for f in $^; do \
			echo "        - $$f";\
		done;\
	    echo "===============================";\
		for f in $^; do \
			dsc_install $$f $$p;\
		done;\
	done
