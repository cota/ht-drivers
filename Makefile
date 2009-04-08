#
# Makefile to build CDCM and driver utils (including XML library)
#
include ./Makefile.specific
include ../makefiles/Kbuild.include

# Quiet
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
	$(MAKE) -C $(KSRC) M=$(PWD) CPU=$(CPU) KVER=$(KVER) modules
	@-rm -r .*.cmd Module.symvers .tmp_versions/

# Driver utilities (including XML library) only
USRC := $(TOPDIR)/utils/driver
utils:
	$(MAKE) -C $(KSRC) M=$(USRC) CPU=$(CPU) KVER=$(KVER) modules
	@-rm -r $(USRC)/.*.cmd $(USRC)/Module.symvers $(USRC)/.tmp_versions/

# Rebuild all, including CDCM, XML library and driver utilities.
all: cdcm utils