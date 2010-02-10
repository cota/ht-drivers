###############################################################################
# @file Makefile
#
# @brief Build generic installation program.
#
# @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
#
# @date Created on 09/06/2009
###############################################################################

# CPU should be defined
$(if $(CPU),,$(error CPU [L865 ppc4] not defined))

# Standart CERN Makefile rules
include /acc/src/dsc/co/Make.auto

# That's our default target when none is given on the command line
PHONY := _all
_all: all

ROOTDIR := ../..

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

OBJFILES := $(SRCFILES:.c=.$(CPU).o)

INCDIRS = ./ $(ROOTDIR) $(ROOTDIR)/include
CFLAGS += $(addprefix -I,$(INCDIRS)) -g

EXEC_OBJS = instprog.$(CPU)

all: $(EXEC_OBJS)

# redefine Make.auto rule to get rid of name dependencies
$(EXEC_OBJS): $(OBJFILES)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	@mkdir -p object/
	mv $(OBJFILES) object

clean clear:
	rm -rf object/ instprog.$(CPU)

__destination = $(filter-out $(strip $(ACCS)) deliver,$(MAKECMDGOALS))
destination := $(if $(__destination), $(__destination), $(ACCS))

myprint:
	@echo "is '$(__destination)'       and      '$(MAKECMDGOALS)'"

# quiet down
#$(destination):
#@:

deliver: FORCE
	@for a in $(destination); do \
		tmp_dir="/acc/dsc/$$a/$(CPU)/bin"; \
		if [ -w $$tmp_dir ]; then \
			echo -e "\nDelivering $(EXEC_OBJS) in $$tmp_dir"; \
			dsc_install $(EXEC_OBJS) $$tmp_dir; \
		elif [ -e $$tmp_dir ]; then \
			echo -e "\nCan't deliver $(EXEC_OBJS) in $$tmp_dir"; \
			echo -e "You don't write permissions!"; \
		fi; \
	done
	@echo ""

PHONY += all
_all: all

PHONY += FORCE
FORCE:

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)