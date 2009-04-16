#
# Makefile to build CDCM and driver utils (including XML library)
#

# Where everything is nested
ROOTDIR = /acc/src/dsc/drivers/coht

include ./Makefile.specific

# Include generic definitions
include $(ROOTDIR)/makefiles/Kbuild.include


# Quiet you...
MAKEFLAGS += --no-print-directory

# Default target is CDCM only
_all: cdcm

#################################################
# Verbose output. Can be used for debugging
# SHELL += -x
# pass SHELL="$(SHELL)" as a parameter to Kmake
#################################################

# CDCM only
cdcm:
	$(MAKE) -C $(KSRC) M=$(PWD) CPU=$(CPU) KVER=$(KVER) \
	ROOTDIR=$(ROOTDIR) modules
	@-rm -r .*.cmd Module.symvers .tmp_versions/

# Driver utilities (including XML library) only
USRC := $(ROOTDIR)/utils/driver
utils:
	$(MAKE) -C $(KSRC) M=$(USRC) CPU=$(CPU) KVER=$(KVER) \
	ROOTDIR=$(ROOTDIR) modules
	@-rm -r $(USRC)/.*.cmd $(USRC)/Module.symvers $(USRC)/.tmp_versions/

# Rebuild all, including CDCM, XML library and driver utilities.
all: cdcm utils