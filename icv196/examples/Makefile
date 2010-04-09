###############################################################################
# @file Makefile
#
# @brief Compile ICV196 library usage example
#
# to compile for Linux -- make
# To compile for Lynx  -- make CPU=ppc4'
#
# @author Yury GEORGIEVSKIY <ygeorgie@gmail.com>
#
# @date Created on 08/04/2010
###############################################################################
CPU ?= L865

# Include standard CERN autorules
include /acc/src/dsc/co/Make.auto

# Options for compiler & linker
CFLAGS = -g -Wall -I../include

LDFLAGS += -L../object_icv196vme
LDLIBS  += -licv196vme.$(CPU)

# sources
SRCS = lib-usage-example.c

all: icv196.$(CPU)

icv196.$(CPU): $(SRCS:.c=.$(CPU).o)
	$(LINK.c) $^  $(LOADLIBES) $(LDLIBS) -o $@
	@rm *.o