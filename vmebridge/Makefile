include common.mk

all: drvr includes lib drvrtest test

drvr:
	cd $(DRVDIR) && $(MAKE)

drvrtest: drvr includes
	$(MAKE) -C $(DRVTESTDIR)

lib:
	$(MAKE) -C $(LIBDIR)

test: drvr includes lib
	$(MAKE) -C $(TESTDIR)

includes:
	$(MAKE) -C $(INCDIR)

clean:
	cd $(DRVDIR) && $(MAKE) clean
	$(MAKE) -C $(DRVTESTDIR) clean
	$(MAKE) -C $(LIBDIR) clean
	$(MAKE) -C $(TESTDIR) clean
	$(MAKE) -C $(INCDIR) clean
	$(RM) $(BAKS)

.PHONY: drvr drvrtest includes lib test clean

