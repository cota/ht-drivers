include /acc/src/dsc/co/Make.auto
include ../Makefile.specific

MY_NAME := $(shell echo $(DRIVER_NAME) | tr "[:upper:]" "[:lower:]")

SRCS := ad9516.c
INCDIR := ../include
OUTDIR ?= ../object_$(MY_NAME)

CFLAGS=	-Wall -g -I$(INCDIR)

LIBAD9516 = libad9516.$(CPU).a

all: $(LIBAD9516) $(SRCS:.c=.$(CPU).o) outdir

.PHONY: all clean outdir

outdir: $(LIBAD9516) $(OUTDIR)
	cp $(LIBAD9516) $(OUTDIR)/

$(OUTDIR):
	mkdir -p $@

ad9516.$(CPU).o: ad9516.c libad9516.h $(INCDIR)/ad9516.h

libad9516.$(CPU).a: ad9516.$(CPU).o
	$(AR) rv $(LIBAD9516) $^
	$(RANLIB) $(LIBAD9516)

clean:
	$(RM) *.a *.o $(BAKS)
