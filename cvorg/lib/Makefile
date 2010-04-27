include /acc/src/dsc/co/Make.auto

SRCS := cvorg.c
INCDIR := ../include
AD9516DIR := ../ad9516
OBJAD9516 := $(AD9516DIR)/ad9516.$(CPU).o
DOXY_DIR := doxygen_output
DOXY_FILES := cvorg.c error.c $(INCDIR)/libcvorg.h $(INCDIR)/cvorg.h
DOXY_INSTDIR := /acc/doc/html/private/coht/doxy/cvorg
EXAMPLES_DIR := examples

CFLAGS := -Wall -g -I$(INCDIR) -I$(AD9516DIR) -I../../include

LIBCVORG = libcvorg.$(CPU).a

all: $(LIBCVORG) $(SRCS:.c=.$(CPU).o) examples

.PHONY: all clean doxy doxy_clean examples

cvorg.$(CPU).o: cvorg.c $(AD9516DIR)/libad9516.h $(INCDIR)/ad9516.h \
	$(INCDIR)/cvorg.h libinternal.h

error.$(CPU).o: error.c $(INCDIR)/libcvorg.h libinternal.h

$(OBJAD9516):
	$(MAKE) -C $(AD9516DIR) CPU=$(CPU)

libcvorg.$(CPU).a: cvorg.$(CPU).o error.$(CPU).o $(OBJAD9516)
	$(AR) rv $(LIBCVORG) $^
	$(RANLIB) $(LIBCVORG)

examples:
	$(MAKE) -C $(EXAMPLES_DIR) CPU=$(CPU)

clean:
	$(MAKE) clean -C $(EXAMPLES_DIR) CPU=$(CPU)
	$(RM) *.a *.o $(BAKS)

doxy: doxy_clean $(DOXY_FILES)
	../../doxy/doxy.sh -n"libcvorg" -o"$(DOXY_DIR)" $(DOXY_FILES)

doxy_clean:
	rm -rf $(DOXY_DIR)

doxy_install: doxy
	mkdir -p $(DOXY_INSTDIR)
	cp -r $(DOXY_DIR)/html/* $(DOXY_INSTDIR)/
