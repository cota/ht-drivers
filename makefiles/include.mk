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

include ../$(ROOTDIR)/makefiles/Makefile.base

# CERN delivery
include ../$(ROOTDIR)/makefiles/deliver.mk

# quet down all the rest
linux lynx all:
	@:
