include common.mk

all: drvr includes lib drvrtest test

drvr:
	$(MAKE) -C $(DRVDIR)

drvrtest: drvr includes
	$(MAKE) -C $(DRVTESTDIR)

lib:
	$(MAKE) -C $(LIBDIR)

test: drvr includes lib
	$(MAKE) -C $(TESTDIR)

includes:
	$(MAKE) -C $(INCDIR)

clean:
	$(MAKE) -C $(DRVDIR) clean
	$(MAKE) -C $(DRVTESTDIR) clean
	$(MAKE) -C $(LIBDIR) clean
	$(MAKE) -C $(TESTDIR) clean
	$(MAKE) -C $(INCDIR) clean
	$(RM) $(BAKS)

.PHONY: drvr drvrtest includes lib test clean

