ifeq ($(CPU),L865)
	SUBDIRS += vmebridge
endif

SUBDIRS += utils

ifeq ($(CPU),L865)
	SUBDIRS += cdcm
endif

SUBDIRS += \
	lib \
	icv196 \
	vd80 \
	xmem \
	mttn \
	ctr

ifeq ($(CPU),L865)
	SUBDIRS += vmod
endif

include ./makefiles/Makefile.base
