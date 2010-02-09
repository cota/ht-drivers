###############################################################################
# @file include.mk
#
# @brief Rules, to deliver header files
#
# @author Copyright (C) 2010 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
#
# @date Created on 03/02/2010
#
# @section license_sec License
#          Released under the GPL
###############################################################################

# Makefile from current directory supress one from upper level
include $(shell if [ -e ./Makefile.specific ]; then \
		echo ./Makefile.specific; \
	else \
		echo ../Makefile.specific; \
	fi)

include ../Makefile

# CERN delivery
include ../$(ROOTDIR)/makefiles/deliver.mk
deliver:
	$(Q)$(MAKE) _deliver $(filter $(strip $(ACCS)),$(MAKECMDGOALS)) CPU=L865
	$(Q)$(MAKE) _deliver $(filter $(strip $(ACCS)),$(MAKECMDGOALS)) CPU=ppc4

# quet down all the rest
linux lynx all:
	@:
