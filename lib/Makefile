ROOTDIR := $(if $(ROOTDIR),$(ROOTDIR),..)

SUBDIRS = \
	stub

# Check if we are in one of the $(SUBDIRS) and do proper include
ifeq (,$(findstring $(notdir $(shell pwd)), $(SUBDIRS)))
  include $(ROOTDIR)/makefiles/Makefile.base
else
  include ../$(ROOTDIR)/makefiles/Makefile.base
endif
