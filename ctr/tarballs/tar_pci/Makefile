#  
# Native compile
#

KVER=$(shell uname -r)

COMPILE_TIME:=$(shell date +%s)

EXTRA_CFLAGS += -DCOMPILE_TIME=$(COMPILE_TIME) -DCTR_PCI -DEMULATE_LYNXOS_ON_LINUX \
	  -I\$(M)/src -DHARD_FLOAT -mhard-float -Wno-strict-prototypes

BAKS = ,* *~ *.bak *.BAK .es1* .B* %*% .ec1 .ek1 .*~ core a.out *JNL *.lst \\\#*\\\# .nfs* *%

SRCS = ctrp.c
PWD:=$(shell pwd)
obj-m   := $(SRCS:.c=.o)

default:
	cp src/Module.c ctrp.c
	rm -f *.o
	$(MAKE) -C /lib/modules/$(KVER)/build M=$(PWD) clean
	$(MAKE) -C /lib/modules/$(KVER)/build M=$(PWD) modules

clean:
	rm -f *.o
	rm -f *.c
	rm -f *.ko
	rm -f ,*
	rm -f Module.*
	rm -f .ctrp*
	rm -rf .tmp_versions
	rm -f .es1
	rm -f ,*
