
#=============================================================================
# SOURCES/OBJECTS/EXECUTABLE
#============================================================================= 
# Sources



HDRS1.COMMON=  libmtt.h asm.h mttRegisters.h opcodes.h
HDRS1.ppc4  = $(HDRS1.COMMON) 
HDRS1.Linux  = $(HDRS1.COMMON) 
HDRS1=$(HDRS1.$(CPU))

HDRS  = $(HDRS1)

SRCS1.ppc4 = libmtt.c opcodes.c
SRCS1.L864 = opcodes.c
SRCS1=$(SRCS1.$(CPU))

SRCS  = $(SRCS1)

# Objects
OBJS1 = $(SRCS1:.c=.$(CPU).o)
OBJS  = $(OBJS1)

# Bins
LIB_MTT.ppc4=libmtt.$(CPU).a
LIB_MTT.L864=libmtt.$(CPU).a
BINS1 = $(LIB_MTT.$(CPU))
BINS  = $(BINS1) 

ALL=$(BINS) 

#=============================================================================
#  LOCAL definitions
#=============================================================================
CFLAGS= -g -Wall -I. -I../driver -I../../include
LDLIBS=-l
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


$(BINS1): $(OBJS1)
	@- rm -f $@;\
	$(AR) $(ARFLAGS) $@ $^;\
	$(RANLIB) $@

INSTALL_FILES=$(BINS1) $(HDRS1)
INSTALL_DIR=mttn
install: $(INSTALL_FILES)
	@p=/ps/local/$(CPU)/$(INSTALL_DIR);\
	echo "===============================";\
	echo "Installing in $$p :";\
	for f in $^; do \
		echo "        - $$f";\
	done;\
    echo "===============================";\
	for f in $^; do \
		dsc_install $$f $$p;\
	done;
	
#=============================================================================
#  DEPENDANCIES
#============================================================================= 
$(SRCS1) : $(HDRS1)
$(OBJS1) : $(SRCS1)

#===============================================================================
# Dependancies
#===============================================================================
# DO NOT DELETE THIS LINE -- make depend depends on it.

