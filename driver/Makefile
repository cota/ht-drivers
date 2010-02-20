#
# Makefile for the VME bridge device driver.
#
include ../common.mk

ACCS := lab oper oplhc
DRVROBJ := $(DRVR).ko

all:
	$(MAKE) -C $(KERN_DIR) M=`pwd`

.PHONY: all clean help deliver $(ACCS)

clean:
	$(MAKE) -C $(KERN_DIR) M=`pwd` clean

help:
	$(MAKE) -C $(KERN_DIR) M=`pwd` help

# first we do some sanity checks on the provided delivery targets
wrong	:= $(filter-out $(strip $(ACCS)) deliver,$(MAKECMDGOALS))
dest	:= $(filter $(strip $(ACCS)),$(MAKECMDGOALS))

# Note that the 'deliver' target depends on 'all'.
# This way we make sure there's a 1:1 match between the CPU/KVER of the vmebus.ko
# binary and the directory to which it will be delivered.
deliver: all
	$(if $(wrong), $(error Wrong delivery targets (valid ones: "$(ACCS)")))
	$(if $(dest),, $(error Empty delivery targets (valid ones: "$(ACCS)")))
	@for complex in $(dest); do \
		destdir="/acc/dsc/$$complex/$(CPU)/$(KVER)/$(DRVR)"; \
		if [ -w $$destdir ]; then \
			echo -e "\nDelivering $(DRVROBJ) to $$destdir"; \
			dsc_install $(DRVROBJ) $$destdir; \
		elif [ ! -e $$destdir ]; then \
			echo -e "\nCreating $$destdir directory..."; \
			mkdir $$destdir; \
			echo -e "Delivering $(DRVROBJ) to $$destdir"; \
			dsc_install $(DRVROBJ) $$destdir; \
		elif [ -e $$destdir ]; then \
			echo -e "\nCan't deliver $(DRVROBJ) to $$destdir"; \
			echo -e "You don't have write permissions!"; \
		fi; \
	done

# There's nothing to build for each CERN complex, but we just put them
# here because otherwise make would complain.
$(ACCS):
	@:

