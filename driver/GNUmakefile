# =====================================================
# Makefile to produce Video Matrix (VMUX) device driver
# =====================================================

include /acc/src/dsc/co/Make.auto

CPU=ppc4

COMPILE_TIME:=$(shell ./gettime)
CFLAGS = -I. $(KERN_INCLUDES) -I/acc/local/$(CPU)/include -DCOMPILE_TIME=$(COMPILE_TIME)
KERN_CFLAGS+=-DCOMPILE_TIME=$(COMPILE_TIME)

DRVR=mttdrvr
DDIR=mtt
HDRS=mtthard

#========================================================================
# compiler to be used to compile drivers
#========================================================================
DRCC=$(KERN_CC)
ifeq ($(CPU),68k)
    POST_DRVR=cp -p $(DRVR).o $@
else
    POST_DRVR=$(KERN_LD) -bM:SRE -bimport:$(DRVR).$(BSP).imp -o $@ $(DRVR).o
    DRCC+=-Wall
endif

all: $(DRVR).$(BSP).$(CPU).o

$(DRVR).$(BSP).$(CPU).o: $(DRVR).c $(DRVR).h $(DRVR)P.h $(HDRS).h
	@$(RM) $(DRVR).o
	$(DRCC) $(KERN_CFLAGS) -c -o $(DRVR).o $(DRVR).c
ifneq ($(CPU),68k)
	$(TOOLS)nm -u $(DRVR).o  | sed '/^[.]/s///' > $(DRVR).$(BSP).imp
else
	sleep 5
endif
	$(POST_DRVR)
	@$(RM) $(DRVR).o


install:
	$(MAKE) CPU=ppc4 install_xxx

install_xxx: $(DRVR).$(BSP).$(CPU).o
	@for a in $(ACCS); do \
		d=/acc/dsc/$$a/$(CPU)/$(BSP)/mtt;\
		f=$(DRVR).$(BSP).$(CPU).o;\
		echo Installing $$f in $$d;\
		dsc_install $$f $$d;\
	done;
	@dsc_install  mttdrvr.h /acc/local/$(CPU)/mtt
	@dsc_install  mtthard.h /acc/local/$(CPU)/mtt
	@dsc_install  mttdrvrP.h /acc/local/$(CPU)/mtt

clean:
	-$(RM) $(DRVR).*.o $(DRVR).*.imp $(BAKS)
