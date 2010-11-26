
CPU=L865
CFLAGS=-g -Wall


ifeq ($(CPU), ppc4)
include /acc/src/dsc/co/Make.auto	
all: tstlibttl.$(CPU) tstlibdor.$(CPU) tstlib12a2.$(CPU) tstlib16a2.$(CPU) tstlib12e16.$(CPU) 
HEADERS=./ \
        -I../include/ \
        -I/acc/src/dsc/drivers/dioaiofclty/include \
        -I/acc/dsc/lab/ppc4/include

LOADLIBES+=-L/ps/local/ppc4/lib/ -L../lib/

LDLIBS=-ldrvrutil -lerr -luio -lvme -lbus

tstlibttl.$(CPU): 
	mkdir -p obj/
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) $^ tstlibttl.c -o ./obj/tstlibttl.o 
	$(LINK.c) -o./obj/$@ ./obj/tstlibttl.o $(LDFLAGS) $^ -lvmodttl2dioaio $(LOADLIBES) $(LDLIBS)

tstlibdor.$(CPU): 
	mkdir -p obj/
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) $^ tstlibdor.c -o ./obj/tstlibdor.o 
	$(LINK.c) -o./obj/$@ ./obj/tstlibdor.o $(LDFLAGS) $^ -lvmoddor2dioaio $(LOADLIBES) $(LDLIBS)

tstlib12a2.$(CPU): 
	mkdir -p obj/
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) $^ tstlib12a2.c -o ./obj/tstlib12a2.o 
	$(LINK.c) -o./obj/$@ ./obj/tstlib12a2.o $(LDFLAGS) $^ -lvmod2dioaio $(LOADLIBES) $(LDLIBS)

tstlib16a2.$(CPU): 
	mkdir -p obj/
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) $^ tstlib16a2.c -o ./obj/tstlib16a2.o 
	$(LINK.c) -o./obj/$@ ./obj/tstlib16a2.o $(LDFLAGS) $^ -lvmod2dioaio $(LOADLIBES) $(LDLIBS)

tstlib12e16.$(CPU):
	mkdir -p obj/
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) $^ tstlib12e16.c -o ./obj/tstlib12e16.o 
	$(LINK.c) -o./obj/$@ ./obj/tstlib12e16.o $(LDFLAGS) $^ -lvmod2dioaio $(LOADLIBES) $(LDLIBS)

else

CC=gcc
HEADERS := ../include/ \
	-I./ \
	-I../driver/

LOADLIBES := -L../lib/
LDLIBS=-lvmodttl 
all: tstlibttl.$(CPU) tstlibdor.$(CPU) load_test.$(CPU) tstlibttl_quick_write.$(CPU) tstlibttl_quick_read.$(CPU) \
	tstlib12a2.$(CPU) tstlib16a2.$(CPU) tstlib12e16.$(CPU)

tstlibttl.$(CPU): 
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) tstlibttl.c -o ./obj/tstlibttl.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/tstlibttl.o $(LOADLIBES) $(LDLIBS)

tstlibdor.$(CPU): 
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) tstlibdor.c -o ./obj/tstlibdor.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/tstlibdor.o $(LOADLIBES) -lvmoddor 

tstlibttl_quick_write.$(CPU): 
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) tstlibttl_quick_write.c -o ./obj/tstlibttl_quick_write.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/tstlibttl_quick_write.o $(LOADLIBES) $(LDLIBS) -lpthread

tstlibttl_quick_read.$(CPU): 
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) tstlibttl_quick_read.c -o ./obj/tstlibttl_quick_read.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/tstlibttl_quick_read.o $(LOADLIBES) $(LDLIBS) -lpthread

load_test.$(CPU):
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) load_test.c -o ./obj/load_test.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/load_test.o $(LOADLIBES) $(LDLIBS) -lpthread

tstlib12a2.$(CPU): 
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) tstlib12a2.c -o ./obj/tstlib12a2.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/tstlib12a2.o $(LOADLIBES) -lvmod
tstlib16a2.$(CPU): 
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) tstlib16a2.c -o ./obj/tstlib16a2.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/tstlib16a2.o $(LOADLIBES) -lvmod 
tstlib12e16.$(CPU):
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) tstlib12e16.c -o ./obj/tstlib12e16.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/tstlib12e16.o $(LOADLIBES) -lvmod
endif 

clean:
	rm -rf ./obj/
