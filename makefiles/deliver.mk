###############################################################################
# @file deliver.mk
#
# @brief CERN-specific delivery
#
# @author Copyright (C) 2009-2010 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
#
# @date Created on 28/05/2009
#
# @section license_sec License
#          Released under the GPL
###############################################################################

# We need ACCS
ifeq (, $(ACCS))
	include /acc/src/dsc/co/Make.common
endif

# Major delivery pathes
UTILINSTDIR = /acc/local/$(CPU)/drv
EXECINSTDIR = /acc/dsc

wrong-dest := $(filter-out $(strip $(ACCS)) _deliver all,$(MAKECMDGOALS))
__destination = $(filter $(strip $(ACCS)),$(MAKECMDGOALS))
destination := $(if $(__destination), $(__destination),)

# quiet down
$(destination):
	@:

ifeq ($(notdir $(shell pwd)), driver)
_deliver:
	$(if $(wrong-dest), \
		$(error wrong delivery place(s) "$(wrong-dest)"),)
	$(if $(destination),, \
		$(error you should explicitly specify destination "$(ACCS)"))
	@for a in $(destination); do \
		tmp_dir="$(EXECINSTDIR)/$$a/$(CPU)/$(KVER)/$(DRIVER_NAME)"; \
		if [ -w $$tmp_dir ]; then \
			echo -e "\nDelivering $(notdir $(DRIVER)) in $$tmp_dir"; \
			dsc_install $(DRIVER) $$tmp_dir; \
		elif [ ! -e $$tmp_dir ]; then \
			echo -e "\nCreating $$tmp_dir directory..."; \
			mkdir $$tmp_dir; \
			echo -e "Delivering $(notdir $(DRIVER)) in $$tmp_dir"; \
			dsc_install $(DRIVER) $$tmp_dir; \
		elif [ -e $$tmp_dir ]; then \
			echo -e "\nCan't deliver $(notdir $(DRIVER)) in $$tmp_dir"; \
			echo -e "You don't have write permissions!"; \
		fi; \
	done
	@echo ""
else ifeq ($(notdir $(CURDIR)), test)
# Deliver test program
_deliver:
	$(if $(wrong-dest), \
		$(error wrong delivery place(s) "$(wrong-dest)"),)
	$(if $(destination),, \
		$(error you should explicitly specify destination "$(ACCS)"))
	@for a in $(destination); do \
		tmp_dir="$(EXECINSTDIR)/$$a/$(CPU)/bin"; \
		if [ -w $$tmp_dir ]; then \
			echo -en "\nDelivering $(EXEC_OBJS) in $$tmp_dir -- "; \
			dsc_install ../$(FINAL_DEST)/$(EXEC_OBJS) $$tmp_dir; \
			echo -e "Done"; \
		fi; \
	done
	@echo ""
else ifeq ($(notdir $(CURDIR)), lib)
# Deliver .a
INSTDIR = $(UTILINSTDIR)
_deliver:
	$(if $(wrong-dest), \
		$(error wrong delivery place(s) "$(wrong-dest)"),)
	@ umask 0002; \
	if [ ! -d $(INSTDIR)/$(DRIVER_NAME) ]; then \
		echo "Creating $(INSTDIR)/$(DRIVER_NAME) lib directory..."; \
		mkdir -p $(INSTDIR)/$(DRIVER_NAME); \
	fi; \
	echo "Copying new $(LIBRARIES) libraries to '$(INSTDIR)/$(DRIVER_NAME)' directory..."; \
	dsc_install $(addprefix ../$(FINAL_DEST)/, $(LIBRARIES)) $(INSTDIR)/$(DRIVER_NAME)
else ifeq ($(notdir $(CURDIR)), include)
# Deliver .h
INSTDIR = $(UTILINSTDIR)
_deliver:
	@ umask 0002; \
	if [ ! -d $(INSTDIR)/$(DRIVER_NAME) ]; then \
		echo "Creating $(INSTDIR)/$(DRIVER_NAME) include directory..."; \
		mkdir -p $(INSTDIR)/$(DRIVER_NAME); \
	fi; \
	echo "Copying new header files to '$(INSTDIR)/$(DRIVER_NAME)' directory..."; \
	dsc_install *.h $(INSTDIR)/$(DRIVER_NAME);
endif

deliver:
	$(Q)$(MAKE) _deliver $(filter-out $@, $(MAKECMDGOALS)) CPU=$(CPU)