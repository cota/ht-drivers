###############################################################################
# @file Kbuild
#
# @brief General Kbuild file.
#
# @author Yury GEORGIEVSKIY, CERN.
#
# @date Created on 13/01/2009
#
# Based on current CPU setting - will include proper specific Kbuild.
###############################################################################

ifeq ($(CPU), Lces)
	include $(M)/../../makefiles/Kbuild.linux
endif

ifeq ($(CPU), L864)
	include $(M)/../../makefiles/Kbuild.linux
endif

ifeq ($(CPU), L865)
	include $(ROOTDIR)/makefiles/Kbuild.linux
endif

ifeq ($(CPU), ppc4)
	include ../../makefiles/Kbuild.lynx
endif
