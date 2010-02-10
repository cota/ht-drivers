###############################################################################
# @file Makefile.tst
#
# @brief Builds up test programs.
#
# @author Yury GEORGIEVSKIY, CERN.
#
# @date Created on 13/01/2009
###############################################################################

# Makefile from current directory supress one from upper level
include $(shell if [ -e ./Makefile.specific ]; then \
		echo ./Makefile.specific; \
	else \
		echo ../Makefile.specific; \
fi)

include ../Makefile

vpath %.c ./  ../../utils/user ../../utils/extest

ADDCFLAGS  = $(STDFLAGS) -DDRIVER_NAME=\"$(DRIVER_NAME)\"

# libraries (and their pathes) to link executable file with
XTRALIBDIRS = ../$(ROOTDIR)/utils/user/object ../$(FINAL_DEST)
LOADLIBES  := $(addprefix -L,$(XTRALIBDIRS)) $(LOADLIBES) -lutils.$(CPU) \
		-lxml2 -lz -ltermcap

# Get all local libs (in object_ directory) user wants to compile with
LOCAL_LIBS = $(patsubst ../$(FINAL_DEST)/lib%.a, -l%, $(wildcard ../$(FINAL_DEST)/*.$(CPU).a))
XTRALIBS   = -lreadline
LDLIBS     = \
	   $(LOCAL_LIBS) \
	   $(XTRALIBS)

SRCFILES = $(wildcard *.c)

# the standard test program (utils/extest) will be compiled
# unless USE_EXTEST is set to 'n'
ifneq ($(USE_EXTEST), n)
SRCFILES += \
	extest.c

# if the driver is skel, we'll compile in all the skel handlers
ifeq ($(IS_SKEL), y)
SRCFILES += cmd_skel.c
else
# if not, then the generic ones are taken to handle built-in commands
SRCFILES += cmd_generic.c
endif

endif # end USE_EXTEST

ifeq ($(CPU), ppc4)
SRCFILES    += extra_for_lynx.c
else
LOADLIBES += -lrt
endif

INCDIRS = \
	./ \
	../.. \
	../driver \
	../include \
	../../utils \
	../../utils/user \
	../../include \
	../../utils/extest \
	/acc/local/$(CPU)/include

EXEC_OBJS = $(DRIVER_NAME)Test.$(CPU)

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
move_objs:
	$(Q)mv $(OBJFILES) $(OBJDIR)
	$(Q)mv $(EXEC_OBJS) ../$(FINAL_DEST)

# CERN delivery
include ../$(ROOTDIR)/makefiles/deliver.mk
deliver:
	$(Q)$(MAKE) _deliver $(filter $(strip $(ACCS)) all, $(MAKECMDGOALS)) CPU=L865
	$(Q)$(MAKE) _deliver $(filter $(strip $(ACCS)) all, $(MAKECMDGOALS)) CPU=ppc4

cleanloc clearloc:: abort
	@ if [ -n "$(OBJDIR)" ]; then \
		rm -rf $(OBJDIR)* ; \
	fi
	-rm -f ../$(FINAL_DEST)/testprog $(DRIVER_NAME)Tst
	-find ./ -name '*~' -o -name '*.$(CPU).o' | xargs rm -f
