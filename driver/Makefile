#
# Makefile for the VME bridge device driver.
#

obj-m = vmebus.$(KVER).o
vmebus.$(KVER)-objs	:= tsi148.o vme_irq.o vme_window.o vme_dma.o	\
			   vme_misc.o vme_cesif.o vme_bridge.o

clean:
	$(RM) *~ \#*\#
