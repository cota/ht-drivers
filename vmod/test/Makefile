
CPU=L865
CFLAGS=-g -Wall


ifeq ($(CPU), ppc4)
include /acc/src/dsc/co/Make.auto	
all: test_vmodttl.$(CPU) test_vmoddor.$(CPU) tstlib12a2.$(CPU) tstlib16a2.$(CPU) tstlib12e16.$(CPU) 
HEADERS=./ \
        -I../include/ \
        -I/acc/src/dsc/drivers/dioaiofclty/include \
        -I/acc/dsc/lab/ppc4/include

LOADLIBES+=-L/ps/local/ppc4/lib/ -L../lib/

LDLIBS=-ldrvrutil -lerr -luio -lvme -lbus

test_vmodttl.$(CPU): 
	mkdir -p obj/
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) $^ test_vmodttl.c -o ./obj/test_vmodttl.o 
	$(LINK.c) -o./obj/$@ ./obj/test_vmodttl.o $(LDFLAGS) $^ -lvmodttl2dioaio $(LOADLIBES) $(LDLIBS)

test_vmoddor.$(CPU): 
	mkdir -p obj/
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) $^ test_vmoddor.c -o ./obj/test_vmoddor.o 
	$(LINK.c) -o./obj/$@ ./obj/test_vmoddor.o $(LDFLAGS) $^ -lvmoddor2dioaio $(LOADLIBES) $(LDLIBS)

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
all: test_vmodttl.$(CPU) test_vmoddor.$(CPU) load_test.$(CPU) test_quick_write.$(CPU) test_quick_read.$(CPU) \
	tstlib12a2.$(CPU) tstlib16a2.$(CPU) tstlib12e16.$(CPU)

test_vmodttl.$(CPU): 
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) test_vmodttl.c -o ./obj/test_vmodttl.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/test_vmodttl.o $(LOADLIBES) $(LDLIBS)

test_vmoddor.$(CPU): 
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) test_vmoddor.c -o ./obj/test_vmoddor.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/test_vmoddor.o $(LOADLIBES) -lvmoddor 

test_quick_write.$(CPU): 
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) test_quick_write.c -o ./obj/test_quick_write.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/test_quick_write.o $(LOADLIBES) $(LDLIBS) -lpthread

test_quick_read.$(CPU): 
	mkdir -p obj
	$(CC) $(CFLAGS) -c -D$(CPU) -I$(HEADERS) test_quick_read.c -o ./obj/test_quick_read.o
	$(CC) $(CFLAGS) -o ./obj/$@ ./obj/test_quick_read.o $(LOADLIBES) $(LDLIBS) -lpthread

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
