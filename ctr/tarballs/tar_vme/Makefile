#  
# Native compile
#

DRVR=ctrv

KVER=$(shell uname -r)

COMPILE_TIME:=$(shell date +%s)

EXTRA_CFLAGS += -DCOMPILE_TIME=$(COMPILE_TIME) -DCTR_VME -DEMULATE_LYNXOS_ON_LINUX \
	  -I\$(M)/src -DHARD_FLOAT -mhard-float -Wno-strict-prototypes

BAKS = ,* *~ *.bak *.BAK .es1* .B* %*% .ec1 .ek1 .*~ core a.out *JNL *.lst \\\#*\\\# .nfs* *%

SRCS = $(DRVR).c
PWD:=$(shell pwd)
obj-m   := $(SRCS:.c=.o)

default:
	cp Module.symvers.ctrv Module.symvers
	chmod 666 Module.symvers
	cp src/Module.c $(DRVR).c
	rm -f *.o
	$(MAKE) -C /lib/modules/$(KVER)/build M=$(PWD) modules

clean:
	rm -f $(BAKS)
	rm -f $(DRVR).*
	rm -f $(DRVR).ko
	rm -f Module.symvers
	rm -f .$(DRVR)*
	rm -rf .tmp_versions
	rm -f modules.order
