CPU ?= L865

include /acc/src/dsc/co/Make.auto

VPATH = ../../utils/extest ../lib
INCDIR := ../include
CFLAGS := -Wall -g -I$(INCDIR) -I../../include -I../../utils/extest -I../lib \
	-I../../utils/user -I/acc/local/$(CPU)/include -DDRIVER_NAME=\"sis33\"
LIBUTILSPATH := ../../utils/user
LIBUTILS := $(LIBUTILSPATH)/object/libutils.$(CPU).a
LDLIBS := $(LIBUTILS)
LDFLAGS := -lxml2 -lz -ltermcap -lreadline -lrt
HSIS33 := $(INCDIR)/sis33.h

all: sis33test.$(CPU)

.PHONY: all clean

sis33test.$(CPU): sis33test.$(CPU).o sis33acq.$(CPU).o sis33dev.$(CPU).o \
	extest.$(CPU).o cmd_generic.$(CPU).o $(LIBUTILS)
sis33test.$(CPU).o: sis33test.c $(HSIS33) extest.h
sis33acq.$(CPU).o: sis33acq.c sis33acq.h $(HSIS33)
sis33dev.$(CPU).o: sis33dev.c sis33dev.h $(HSIS33)
extest.$(CPU).o: extest.c extest.h extest_common.h
cmd_generic.$(CPU).o: cmd_generic.c extest.h extest_common.h cmd_handlers.h

$(LIBUTILS):
	$(MAKE) -C $(LIBUTILSPATH) CPU=$(CPU)

clean:
	$(RM) *.$(CPU) *.o *.$(CPU).a $(BAKS)
