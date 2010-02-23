include ../common.mk
include /acc/src/dsc/co/Make.auto


all: clean
	ln -s ../$(DRVDIR)/vmebus.h vmebus.h
	ln -s ../$(LIBDIR)/libvmebus.h libvmebus.h

.PHONY: all clean deliver

clean:
	rm -f vmebus.h libvmebus.h
	$(RM) $(BAKS)

deliver: all
	@ umask 0002; \
	if [ ! -d $(INSTDIR) ]; then \
		echo "Creating $(INSTDIR)"; \
		mkdir -p $(INSTDIR); \
	fi; \
	echo "Copying vmebus.h and libvmebus.h to $(INSTDIR)"; \
	dsc_install vmebus.h libvmebus.h $(INSTDIR)

