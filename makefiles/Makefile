###############################################################################
# @file Makefile
#
# @brief Makefile to compile module benchmark.
#
# It includes driver, libraries and test programs.
#
# @author Yury GEORGIEVSKIY, CERN.
#
# @date Created on 13/01/2009
###############################################################################

# User-specific settings
include Makefile.specific

# Where everything is nested
# User can provide it's own, otherwise default will be used
ROOTDIR := $(if $(ROOTDIR),$(ROOTDIR),..)

# Standard driver directory tree layout
SUBDIRS = \
	lib \
	driver \
	test \
	include

include $(ROOTDIR)/makefiles/Makefile.base