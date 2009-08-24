#================================================================
# Makefile to produce bst Installation program
#================================================================

include /ps/dsc/src/co/Make.auto

CFLAGS= -g -Wall -I/ps/local/$(CPU)/include/tgm -I../driver -I. -DPROTOTYPE
LDLIBS=

SRCS=mttinstall.c drvr_parser.c

ALL=mttinstall.$(CPU)

all:$(ALL)

install:   $(ALL)
	@for a in $(ACCS); do \
		d=/acc/dsc/$$a/$(CPU)/$(BSP)/mtt;\
		f=mttinstall.$(CPU);\
		echo Installing $$f in $$d;\
		dsc_install $$f $$d;\
	done;

depend: $(SRCS1)
	mkdepend -o .'$$(CPU)'.o -- $(STDFLAGS)  -- $(SRCS)

clean:
	$(RM) mttinstall.$(CPU) *.o *.old $(BAKS)


mttinstall.$(CPU): $(SRCS:.c=.$(CPU).o)


# DO NOT DELETE THIS LINE -- make depend depends on it.

