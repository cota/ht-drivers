DOXY_INSTPATH = /acc/doc/html/private/coht/doxy/vd80/driver

all:
	echo Make documentation for VD80 driver
	rm -f ,*.h
	rm -rf doc
	../../doxy/doxy.sh -n"VD80 Device Driver" -o "doc" vd80.doxygen

doxy_install: all
	mkdir -p $(DOXY_INSTPATH)
	cp -r doc/html/* $(DOXY_INSTPATH)
