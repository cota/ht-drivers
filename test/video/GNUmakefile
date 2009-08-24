#================================================================
# Makefile to produce MTT Multitasking CTG test program
#================================================================

include /ps/dsc/src/co/Make.auto

ACCS=tst mcr

CFLAGS = -g -Wall -I. -I../../lib -I../../driver -I$(PS) \
	 -I/ps/local/$(CPU)/include -lX11 -I/tmp
LDLIBS = ../../lib/libmtt.$(CPU).a -ltim -ltgv -ltgm -lerr -llynx -lX11 -lnetinet -lm
ALL  = genvid.$(CPU)
SRCS = genvid.c
HDRS =

all: $(ALL)

clean:
	$(RM) $(ALL) $(BAKS)

# Run on Workstation only

genvid.$(CPU).o: $(SRCS) $(HDRS)

genvid.$(CPU): genvid.$(CPU).o

install: genvid.$(CPU)
	dsc_install genvid.$(CPU) /ps/dsc/tst/$(CPU)/ces/mtt
	dsc_install genvid.$(CPU) /ps/dsc/mcr/$(CPU)/ces/mtt
