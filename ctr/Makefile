CPU = L865

ROOTDIR := $(if $(ROOTDIR),$(ROOTDIR),..)

ifeq ($(CPU), L865)
SUBDIRS += \
	driver_pci \
	driver_vme
endif

# Check if we are in one of the $(SUBDIRS) and do proper include
ifeq (,$(findstring $(notdir $(shell pwd)), $(SUBDIRS)))
  include $(ROOTDIR)/makefiles/Makefile.base
else
  include ../$(ROOTDIR)/makefiles/Makefile.base
endif
