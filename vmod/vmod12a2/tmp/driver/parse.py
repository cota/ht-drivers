#!	/usr/bin/env python

import os
import sys
from xml.dom import minidom

def parse(xmlfilename, driver_name):

    tree = minidom.parse(file(xmlfilename))
    
    drivers = tree.getElementsByTagName('driver')
    vmod12a2 = [ d for d in drivers if d.getAttribute('name') ==
    driver_name ]
    pci_modules = [ p for p in vmod12a2[0].getElementsByTagName('module') if 
                            p.getAttribute('bus_type') == "PCI" ]
    result = []
    for module in pci_modules:
        lun   = module.getAttribute('logical_module_number')
        extra = module.getAttribute('extra')
        pci = module.getElementsByTagName('pci')[0]
        bus_number  = pci.getAttribute('bus_number')
        slot_number = pci.getAttribute('slot_number')
        result.append('%s,%s,%s,%s' % (lun, 
                                       bus_number, 
									   slot_number, 
									   extra))
    return 'insmod vmod12a2.ko luns=' + ','.join(result)

def install_driver(xmlfile, driver_name):

    command = parse(xmlfile, driver_name)
    print('Trying ' + command)
    err = os.system(command)
    if err != 0:
    	print '[%s] failed with error %d' % (command, err)
    	sys.exit(1)
    
    devnos = [ l.split()[0] for l in file('/proc/devices')  
    			if l.find(driver_name.lower()) != -1 ]
    if len(devnos) != 1:
    	'Wrong number of majors for driver %s: %s' % (
    				driver_name, devnos,)
    	sys.exit(1)
    devno = int(devnos[0])
    if False:
        for minor in range(1,17):
            os.system('mknod /dev/%s.%d c %d %d' % (driver_name.lower(), 
                                                  minor,
                                                  devno,
                                                  minor,))
    mknod = 'mknod /dev/%s c %d %d' % ( driver_name.lower(), devno, 0,)
    print 'Trying [%s]' % mknod
    os.system(mknod)
    print 'Device /dev/%s created'
    print 'Driver %s installed, major number %d' % (driver_name, devno,)

if __name__ == '__main__':
	if (len(sys.argv) != 3):
		print 'usage: install.py xmlfile DRIVERNAME'
		sys.exit(1)
	install_driver(sys.argv[1], sys.argv[2])
