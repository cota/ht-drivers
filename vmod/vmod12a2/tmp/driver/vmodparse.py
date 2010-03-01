#!	/usr/bin/env python

import os
import sys
from xml.dom import minidom

carrier_driver_names = [ 'vmodio', 'mod-pci', ]
mz_driver_names      = [ 'vmod12a2', 'vmod16a2', 'vmod12e16', 'vmodttl' ]
driver_names         = carrier_driver_names + mz_driver_names

class LibinstError(Exception): pass
class StructError(LibinstError): pass

def get_drivers(drivers, name):
    ret =  [ d for d in drivers 
    		if d.getAttribute('name').lower() == name.lower() ]
    return ret

def check_mezzanine(tree):
    if not len(tree) == 1:
    	raise StructError('Exactly one driver per mezzanine')
    d = tree[0]
    modules = d.getElementsByTagName('module')
    cars = [ m for m in modules 
    	if m.getAttribute('bus_type').lower() == 'car' ]
    if len(modules) != len(cars):
        raise StructError( 'All modules in driver %s must '
	        'be of type CAR' %  (d.getAttribute('name')))
    return cars

def get_carrier_spaces(module):
    car = module.getElementsByTagName('carrier')
    if len(car) != 1:
        raise StructError('Exactly one carrier space per module')
    car = car[0]
    carrier_name   = car.getAttribute('carrier_name')
    board_number   = car.getAttribute('board_number')
    board_position = car.getAttribute('board_position')
    return (carrier_name, board_number, board_position)

def parse(xmlfilename, required_driver_names):

    try:
        tree = minidom.parse(file(xmlfilename))
    except:
        print "Invalid XML syntax"
        sys.exit(1)
         
    drivers = tree.getElementsByTagName('driver')

    ret = []
    for type in required_driver_names:
        t = get_drivers(drivers, type)
        print 'Got %d driver(s) of type %s' % (
			len(t), type)
        if len(t) == 0: 
            continue
        modules = check_mezzanine(t)

        result = []
        for module in modules:
            lun    = module.getAttribute('logical_module_number')
            cn, bn, bp = get_carrier_spaces(module)
            result += [lun, cn, bn, bp]
        ret += [ (type, ','.join(result)) ]
    return ret

def install_driver(xmlfile, driver_names):

    for driver, luns in parse(xmlfile, driver_names):
    
        insmod = 'insmod %s.ko luns=%s' % (driver, luns)
        print('Trying ' + insmod)
        err = os.system(insmod)
        if err != 0:
            print '[%s] failed with error %d' % (insmod, err)
            sys.exit(1)
        
        devnos = [ l.split()[0] for l in file('/proc/devices')  
                    if l.find(driver.lower()) != -1 ]
        if len(devnos) != 1:
            'No majors for driver %s: %s' % ( driver, devnos,)
            sys.exit(1)
        devno = int(devnos[0])
        mknod = 'mknod /dev/%s c %d %d' % ( driver.lower(), devno, 0,)
        print 'Trying [%s]' % mknod
        os.system(mknod)
        print 'Device /dev/%s created' % driver
        print 'Driver %s installed, major number %d' % (driver, devno,)

if __name__ == '__main__':
    if (len(sys.argv) < 3):
        print 'usage: install.py xmlfile DRIVERNAME...'
        sys.exit(1)
    try:
        install_driver(sys.argv[1], sys.argv[2:])
    except StructError, se:
        print "Semantic XML error: %s" % se.value
