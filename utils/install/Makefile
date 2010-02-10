###############################################################################
# @file Makefile
#
# @brief Build generic installation program.
#
# @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
#
# @date Created on 09/06/2009
###############################################################################

# Makefile from current directory supress one from upper level
include $(shell if [ -e ./Makefile.specific ]; then \
		echo ./Makefile.specific; \
	else \
		echo ../Makefile.specific; \
	fi)

include ../Makefile


# libraries (and their pathes) to link executable file with
XTRALIBDIRS = ../$(ROOTDIR)/utils/user/object
LOADLIBES  := $(addprefix -L,$(XTRALIBDIRS)) $(LOADLIBES) -lutils.$(CPU)

XTRALIBS    = -lxml2 -lz -g
LDLIBS      = $(XTRALIBS)

SRCFILES = instprog.c inst-utils.c

ifeq ($(CPU), ppc4)
# nothing to include for Lynx
else
SRCFILES += inst-linux.c
endif

EXEC_OBJS = instprog.$(CPU)


$(EXEC_OBJS): $(OBJFILES)

_build: $(EXEC_OBJS) $(OBJDIR) move_objs


linux:
	@echo -e "\nCompiling for Linux:"
	$(Q)$(MAKE) _build CPU=L865

lynx:
	@echo -e "\nCompiling for Lynx:"
	$(Q)$(MAKE) _build CPU=ppc4

all:
	$(Q)$(MAKE) linux
	$(Q)$(MAKE) lynx

# Move compiled files to proper place
move_objs: $(FINAL_DEST)
	$(Q)mv $(OBJFILES) $(OBJDIR)
	$(Q)mv $(EXEC_OBJS) ../$(FINAL_DEST)


INCDIRS = ./ ../$(ROOTDIR) ../$(ROOTDIR)/include
ADDCFLAGS += $(addprefix -I,$(INCDIRS)) -g

clean clear:
	rm -rf object/ instprog.$(CPU)
