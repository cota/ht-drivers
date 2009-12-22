include /acc/src/dsc/co/Make.auto

CFLAGS = -I../ -I../../../include

LDLIBS = -L../ -lm -lutils.$(CPU)

ifeq ($(CPU),L865)
LDLIBS += -g -L/usr/lib -lxml2 -lz
CFLAGS += -I/usr/include/libxml2
else
LDLIBS += -g -L/acc/local/$(CPU)/lib/ -lxml2 -lz
endif

all: test.$(CPU)
