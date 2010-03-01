CFLAGS=-g -Wall

test12e16: test12e16.o libvmod12e16.a
	gcc -o $@ $^

libvmod12e16.a: libvmod12e16.o
	ar r $@ $^
libvmod12e16.o: libvmod12e16.h vmod12e16.h
test12e16.o: libvmod12e16.h

clean:
	rm -f *.o libvmod12e16.a test12e16
