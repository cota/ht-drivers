##############################################################################
# @file Makefile
#
# @brief Makefile to build CDCM and driver utils (including XML library)
#
# @author Copyright (C) 2009 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
#
# @date Created on 26/08/2009
###############################################################################

# Where everything is nested
ROOTDIR = $(CURDIR)/..

include ./Makefile.specific
include $(ROOTDIR)/makefiles/rules.mk

# Include generic definitions
include $(ROOTDIR)/makefiles/Kbuild.include

# Quiet you...
MAKEFLAGS += --no-print-directory

# By default -- Rebuild all, including CDCM, XML library and driver utilities.
_build: utils cdcm

#################################################
# Verbose output. Can be used for debugging
# SHELL += -x
# pass SHELL="$(SHELL)" as a parameter to Kmake
#################################################

# CDCM only
cdcm:
	@-rm -rf ./$(CPU)/$(KVER)
	$(MAKE) -C $(KSRC) M=$(shell pwd) CPU=$(CPU) \
	ROOTDIR=$(ROOTDIR) KVER=$(KVER) modules
	@-rm -r .*.cmd Module.symvers .tmp_versions/
	@-rm -rf ./Module.markers ./modules.order

# Driver utilities (including XML library) only
USRC := $(ROOTDIR)/utils/driver
utils:
	@-rm -rf $(USRC)/$(CPU)/$(KVER)
	@$(USRC)/rmdrvr
	$(MAKE) -C $(KSRC) M=$(USRC) CPU=$(CPU) \
	ROOTDIR=$(ROOTDIR) KVER=$(KVER) modules
	@-rm -rf $(USRC)/.*.cmd $(USRC)/Module.symvers $(USRC)/.tmp_versions/
	@-rm -rf $(USRC)/Module.markers $(USRC)/modules.order

doxy_clean:
	rm -rf man

doxy: doxy_clean
	../doxy/doxy.sh -n"Common Driver Code Manager (aka CDCM)" -o "man" ./
