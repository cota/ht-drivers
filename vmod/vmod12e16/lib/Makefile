CFLAGS=-g -Wall
DOXY_INSTPATH = /acc/doc/html/private/coht/vmod12e16/lib

test12e16: test12e16.o libvmod12e16.a
	gcc -o $@ $^

libvmod12e16.a: libvmod12e16.o
	ar r $@ $^
libvmod12e16.o: libvmod12e16.h vmod12e16.h
test12e16.o: libvmod12e16.h

clean: doxy_clean
	rm -f *.o libvmod12e16.a test12e16

doxy_clean:
	rm -rf doxygen_output

doxy:
	../../../doxy/doxy.sh -n"VMOD 12e16 library"  . ../include

doxy_install: doxy
	mkdir -p $(DOXY_INSTPATH)
	cp -r doxygen_output/html/* $(DOXY_INSTPATH)/
	
