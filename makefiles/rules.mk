###############################################################################
# @file rules.mk
#
# @brief Handles default driver framework target
#
# @author Copyright (C) 2010 CERN. Yury GEORGIEVSKIY <ygeorgie@gmail.com>
#
# @date Created on 29/04/2010
#
# @section license_sec License
#          Released under the GPL
###############################################################################

# If not limited by the user (using TARGET_CPU) -- framework will be compiled for this platforms
DEFAULT_CPU = L865 ppc4

# User can compile for this platforms, if (1) framework compilation is not limited
# by TARGET_CPU and (2) CPU is provided in the command line
SUPPORTED_CPU = L865 ppc4 L864 ces

# User can limit framework compilation using this option
# If user does no limitation -- will compile for default targets
TARGET_CPU ?= $(DEFAULT_CPU)

ifeq ("$(origin CPU)", "command line")

# Check CPU provided and bail out if it is not supported
override CPU := $(strip $(if $(findstring "default", "$(CPU)"), $(DEFAULT_CPU), $(CPU)))
FCPU = $(filter-out $(SUPPORTED_CPU), $(CPU))
$(if $(FCPU), $(error wrong/unsupported cpu "$(FCPU)"))

# User wants to compile for specific platform, so
# limit compilation to one requested by the user
TARGET_CPU := $(filter $(CPU), $(TARGET_CPU))

endif

all:
	@for cpu in $(TARGET_CPU) _dummy_cpu_ ; \
		do \
			if [ $$cpu != _dummy_cpu_ ]; then \
				$(MAKE) CPU=$$cpu _build; \
			fi; \
		done
