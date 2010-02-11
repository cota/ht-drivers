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

-include Makefile.specific

# Where everything is nested
# User can provide it's own, otherwise default will be used
ROOTDIR := $(if $(ROOTDIR),$(ROOTDIR),..)

# Standard driver directory tree layout
SUBDIRS = \
	lib \
	include \
	test \
	driver

# Check if we are in one of the $(SUBDIRS) and do proper include
ifeq (,$(findstring $(notdir $(shell pwd)), $(SUBDIRS)))
  include $(ROOTDIR)/makefiles/Makefile.base
else
  include ../$(ROOTDIR)/makefiles/Makefile.base
endif
