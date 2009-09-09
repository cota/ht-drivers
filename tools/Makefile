
#=============================================================================
# SOURCES/OBJECTS/EXECUTABLE
#=============================================================================
# Sources
HDR_LIBMTT=../lib/libmtt.h

HDRS1.ppc4  = asmP.h   $(HDR_LIBMTT)
HDRS1.L864  = asmP.h  $(HDR_LIBMTT)
HDRS1=$(HDRS1.$(CPU))

HDRS  = $(HDRS1)

SRCS1.ppc4 = asm.c
SRCS1.L864 = asm.c
SRCS1=$(SRCS1.$(CPU))

SRCS2.ppc4 = emu.c
SRCS2.L864 = emu.c
SRCS2=$(SRCS2.$(CPU))

SRCS  = $(SRCS1) $(SRCS2)

# Objects
OBJS1 = $(SRCS1:.c=.$(CPU).o)
OBJS2 = $(SRCS2:.c=.$(CPU).o)
OBJS  = $(OBJS1) $(OBJS2)

# Bins
BINS1 = $(OBJS1:.$(CPU).o=.$(CPU))
BINS2 = $(OBJS2:.$(CPU).o=.$(CPU))
BINS  = $(BINS1) $(BINS2)

ALL=$(BINS)

#=============================================================================
#  LOCAL definitions
#=============================================================================
CFLAGS= -g -Wall -I. -I../lib -I../../include -I../driver
LDLIBS=../lib/libmtt.$(CPU).a

#=============================================================================
#  INCLUDES
#=============================================================================
include /acc/dsc/src/co/Make.auto

#=============================================================================
#  TARGETS
#=============================================================================
help:
	@echo "make all --> Build all"
	@echo "make clean --> Clean binary and temporary files"

all: $(ALL)

clean:
	$(RM) $(OBJS) $(ALL) $(BAKS)

INSTALL_FILES=$(BINS)
INSTALL_DIR=mttn
install: $(INSTALL_FILES)
	@for a in $(ACCS); do \
		p=/acc/dsc/$$a/$(CPU)/ces/$(INSTALL_DIR);\
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

#=============================================================================
#  DEPENDANCIES
#=============================================================================
$(SRCS1) : $(HDRS1)
$(OBJS1) : $(SRCS1)
$(BINS1):  $(OBJS1)

#===============================================================================
# Dependancies
#===============================================================================
# DO NOT DELETE THIS LINE -- make depend depends on it.

