DOXY_INSTPATH = /acc/doc/html/private/coht/doxy/vd80/lib

all:
	echo Make documentation for stub library
	rm -f ,*.h
	rm -rf doc
	../../../doxy/doxy.sh -n"Stub dynamic library loader" -o "doc" stub.doxygen

doxy_install: all
	mkdir -p $(DOXY_INSTPATH)
	cp -r doc/html/* $(DOXY_INSTPATH)/
