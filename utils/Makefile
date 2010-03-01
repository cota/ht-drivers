ROOTDIR := $(if $(ROOTDIR),$(ROOTDIR),..)

SUBDIRS = \
	user \
	install

# Check if we are in one of the $(SUBDIRS) and do proper include
ifeq (,$(findstring $(notdir $(shell pwd)), $(SUBDIRS)))
  include $(ROOTDIR)/makefiles/Makefile.base
else
  include ../$(ROOTDIR)/makefiles/Makefile.base
endif
