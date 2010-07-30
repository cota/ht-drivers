
include /ps/dsc/src/co/Make.auto

CFLAGS= -g -Wall -I/ps/local/$(CPU)/include -I../../include -I.

LDLIBS=../../object_icv196vme/libicv196vme.$(CPU).a

SRCS = test.c
HDRS =

ALL  = test.$(CPU)

all: $(ALL)

test.$(CPU).o: $(SRCS) $(HDRS)
