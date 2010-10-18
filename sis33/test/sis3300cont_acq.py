#!/usr/bin/python
# It needs Python 2.7 or higher!

from ctypes import *
from multiprocessing import Process
import string
import optparse
import time

# -------------------------------------------------------------
#		Preliminary declarations
# -------------------------------------------------------------
class SIS33_ACQ (Structure):
    _fields_ = [("data",POINTER(c_uint16)),
                ("nr_samples" , c_uint32),
                ("prevticks", c_uint64),
                ("size", c_uint32)]

class TIMEVAL (Structure):
    _fields_ = [("tv_sec", c_uint32),
                ("tv_usec", c_uint32)]

def print_header():
    print ('')
    print ('\t SIS3300 Testing program \t')
    print ('\t for continous acquisition \t')
    print ('')
    print ('Author: Samuel I. Gonsalvez - BE/CO/HT, CERN ')
    print ('Version: 1.0')
    print ('License: 3-clause BSD license.')
    print ('')
    raise SystemExit

def check_arguments(lun, chan):
    if chan < 0 or chan > 7 :
       print "Error invalid channel:", chan
       raise SystemExit

    f = open('/sys/class/sis33/sis33.' + repr(lun) + '/description', 'r')
    desc = f.readline()
    f.close()
    if not "SIS3300" in desc :
        print "It is not a SIS3300:", desc
        raise SystemExit
        
 
def config_sis(device, chan):
    libsis.sis33_set_clock_source(device, 1) # External clock
    libsis.sis33_set_start_auto(device, 1)
    libsis.sis33_set_stop_auto(device, 1)
    libsis.sis33_set_start_delay(device, 0)
    libsis.sis33_set_stop_delay(device, 0)
   
def acquisition(device, lun, chan, segment, n_acqs, path, num_samp):
    puntero = POINTER(SIS33_ACQ)
    libsis.sis33_acqs_zalloc.restype = puntero
    acqs = libsis.sis33_acqs_zalloc(1, num_samp)
   
    endtime = TIMEVAL()
    fh = None
    s = '/tmp/sis33loop.' + repr(lun) +'.ch'+repr(chan) +'.seg'+repr(segment)+'.num'+repr(0)+'.dat'
    t1 = time.time()
    print "Using", num_samp,"samples for each file"
    print "Start time: ", t1
    for i in range(0,n_acqs/2):
        if libsis.sis33_acq_wait(device, 0, 1, num_samp) < 0 :
            libsis.sis33_perror("sis33_acq_wait")
            found_error(device)

        if libsis.sis33_acq(device, 1, 1, num_samp) < 0 :
            libsis.sis33_perror("sis33_acq_wait")
            found_error(device)

        if libsis.sis33_fetch(device, 0, chan, byref(acqs[0]), 1, byref(endtime)) < 0 :
            libsis.sis33_perror("sis33_fetch")
            found_error(device)

        p = Process(target=write_data, args=(acqs[0].data, 131072, i, lun, chan, 0, path))
        p.start()

        if libsis.sis33_fetch_wait(device, 1, chan, byref(acqs[0]), 1, byref(endtime)) < 0 :
            libsis.sis33_perror("sis33_fetch")
            found_error(device)

        p = Process(target=write_data, args=(acqs[0].data, 131072, i, lun, chan, 1, path))
        p.start()

    t2 = time.time()
    print "Finish time: ", t2
    print "Acquisition Time:", (t2 - t1), "seconds"
    libsis.sis33_acqs_free(acqs, 1)

def calculate_time(time, freq, num_samp):
    tmp = freq*time
    tmp = tmp / num_samp  + 1
    return tmp

def write_data(data, size, num_fich, lun, chan, segment, path):
    s = path+'/sis33loop.' + repr(lun) +'.ch'+repr(chan) +'.seg'+repr(segment)+'.num'+repr(num_fich)+'.dat'
    f = open(s, 'w')
    f.write("#event segment " + repr(segment) + "\n")
    for i in range(0, size-1):
        s = hex(data[i]) + '\n'
        f.write(s)

def found_error(device):
    libsis.sis33_acq_cancel(device)
    exit_program(device)

def exit_program(device):
    libsis.sis33_close(device)
    raise SystemExit

if __name__ == '__main__' :

    parser = optparse.OptionParser()
    parser.add_option("-d", "--device", dest="lun", type="int",
        help =("Logical Unit Number of the SIS33xx device. [default: %default]"))
    parser.add_option("-c", "--channel", dest="chan", type="int",
        help =("Channel to setup the acquisition. [default: %default]"))
    parser.add_option("-t", "--time", dest="time", type="int",
        help =("Time acquisition in seconds. [default: %default]"))
    parser.add_option("-p", "--path", dest="path", type="str",
        help =("Path to save the data. [default: %default]"))
    parser.add_option("-f", "--frequency", dest="freq", type="int",
        help =("Frequency of external clock [Hz]. [default: %default]"))
    parser.add_option("-l", "--lib", dest="lib", type="str",
        help =("Path to the shared library libsis33.L865.so. [default: %default]"))
    parser.add_option("-v", "--version", action="store_true", dest="version")
    parser.set_defaults(lun=0, chan=0, time=1, version=0, path='/tmp', freq=1000000, lib="../lib")
    opts, args = parser.parse_args()

    s = opts.lib + '/libsis33.L865.so'
    libsis = cdll.LoadLibrary(s);

    if opts.version :
        print_header()

    check_arguments(opts.lun, opts.chan)

    device = libsis.sis33_open (opts.lun)
    config_sis(device, opts.chan)

    num_samp = 131072
    n_acqs = calculate_time(opts.time, opts.freq, num_samp)
    acquisition(device, opts.lun, opts.chan, 0, n_acqs, opts.path, num_samp)

    exit_program(device)
