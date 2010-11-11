CPU=L865


all: lib


ifeq ($(CPU), ppc4)
include /acc/src/dsc/co/Make.auto

LIBS=libvmod2dioaio.a libvmodttl2dioaio.a libvmoddor2dioaio.a
lib: $(LIBS)
OBJS=libvmod12a22dioaio.o libvmod16a22dioaio.o libvmod12e162dioaio.o

HEADERS=../include \
        -I/acc/src/dsc/drivers/dioaiofclty/include \
        -I/acc/src/dsc/drivers/ioconfig/include \
        -I/acc/src/dsc/drivers/rt/include/ \
        -I/acc/dsc/lab/ppc4/include

CFLAGS=-g -Wall

libvmod12a22dioaio.o:
	$(CC) $(CFLAGS) -c -I$(HEADERS) libvmod12a22dioaio.c -o $@
libvmod16a22dioaio.o:
	$(CC) $(CFLAGS) -c -I$(HEADERS) libvmod16a22dioaio.c -o $@
libvmod12e162dioaio.o:
	$(CC) $(CFLAGS) -c -I$(HEADERS) libvmod12e162dioaio.c -o $@

libvmodttl2dioaio.o:
	$(CC) $(CFLAGS) -c -I$(HEADERS) libvmodttl2dioaio.c -o $@
libvmoddor2dioaio.o:
	$(CC) $(CFLAGS) -c -I$(HEADERS) libvmoddor2dioaio.c -o $@

libvmod2dioaio.so: $(OBJS)
	gcc -shared -o $@ $^
libvmod2dioaio.a: $(OBJS)
	ar r $@ $^

libvmod12a22dioaio.a: libvmod12a22dioaio.o
	ar r $@ $^
libvmod16a22dioaio.a: libvmod16a22dioaio.o
	ar r $@ $^
libvmod12e162dioaio.a: libvmod12e162dioaio.o
	ar r $@ $^

libvmodttl2dioaio.a: libvmodttl2dioaio.o
	ar r $@ $^
libvmodttl2dioaio.so: libvmodttl2dioaio.o
	$(CC) -shared -o $@ $^
libvmoddor2dioaio.a: libvmoddor2dioaio.o
	ar r $@ $^
libvmoddor2dioaio.so: libvmoddor2dioaio.o
	$(CC) -shared -o $@ $^

else


LIBS=libvmod.a libvmod.so libvmodttl.a libvmoddor.a 
lib: $(LIBS)
OBJS=libvmod12a2.o libvmod16a2.o libvmod12e16.o

HEADERS=../include \
	-I../driver
CFLAGS=-g -Wall

libvmod12a2.o:
	$(CC) $(CFLAGS) -c -I$(HEADERS) libvmod12a2.c -o $@
libvmod16a2.o:
	$(CC) $(CFLAGS) -c -I$(HEADERS) libvmod16a2.c -o $@
libvmod12e16.o:
	$(CC) $(CFLAGS) -c -I$(HEADERS) libvmod12e16.c -o $@
libvmodttl.o: 
	$(CC) $(CFLAGS) -c -I$(HEADERS) libvmodttl.c -o $@
libvmoddor.o: 
	$(CC) $(CFLAGS) -c -I$(HEADERS) libvmoddor.c -o $@

libvmod.so: $(OBJS)
	gcc -shared -o $@ $^
libvmod.a: $(OBJS)
	ar r $@ $^

libvmod12a2.a: libvmod12a2.o
	ar r $@ $^
libvmod16a2.a: libvmod16a2.o
	ar r $@ $^
libvmod12e16.a: libvmod12e16.o
	ar r $@ $^

libvmodttl.a: libvmodttl.o
	ar r $@ $^
libvmodttl.so: libvmodttl.o
	gcc -shared -o $@ $^
libvmoddor.a: libvmoddor.o
	ar r $@ $^
libvmoddor.so: libvmoddor.o
	gcc -shared -o $@ $^

endif

clean:
	rm -f *.o *.a *.pyc *.so
