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
	@-rm -rf ./$(CPU)/$(KVER)
	$(MAKE) -C $(KSRC) M=$(PWD) CPU=$(CPU) \
	ROOTDIR=$(ROOTDIR) modules
	@-rm -r .*.cmd Module.symvers .tmp_versions/
	@-rm -rf ./Module.markers ./modules.order

# Driver utilities (including XML library) only
USRC := $(ROOTDIR)/utils/driver
utils:
	@-rm -rf $(USRC)/$(CPU)/$(KVER)
	@$(USRC)/rmdrvr
	$(MAKE) -C $(KSRC) M=$(USRC) CPU=$(CPU) \
	ROOTDIR=$(ROOTDIR) modules
	@-rm -rf $(USRC)/.*.cmd $(USRC)/Module.symvers $(USRC)/.tmp_versions/
	@-rm -rf $(USRC)/Module.markers $(USRC)/modules.order

# Rebuild all, including CDCM, XML library and driver utilities.
all: utils cdcm