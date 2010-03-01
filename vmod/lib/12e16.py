#!   /usr/bin/env   python
#    coding: utf8

import optparse
import math
import time
import sys
import pyvmod

T = 0.05 

def wave(t, period):
    return math.sin(2*math.pi*t/period)

def dac(digital):
    return 20.0 * (digital / float(1<<12)) - 10.0

if __name__ == '__main__':

    parser = optparse.OptionParser()
    parser.add_option("-l", "--lun", dest="lun", type="int",
        help="logical module number", default=1)
    parser.add_option("-c", "--channel", dest="channel", type="int",
        help="channel (0 or 1)", default=0)
    parser.add_option("-a", "--amplification", dest="ampli", type="int",
        help="amplification factor (0..3)", default=0)
    parser.add_option("-s", "--sampling", dest="sampling", type="float",
        help="sampling frequency in Hz", default=20)
    parser.add_option("-o", "--output", dest="outputfile", type="string",
        help="output file name (default 12e16.out", default='12e16.out')
    parser.add_option("-n", "--number-of-samples", dest="number_of_samples", type="int",
        help="number of samples (default 10K)", default=10000)

    (options, args) = parser.parse_args()
    locals().update(options.__dict__)

    device = '12e16'
    get_handle = pyvmod.__dict__.get('vmod%s_get_handle' % device)
    convert    = pyvmod.__dict__.get('vmod%s_convert'    % device)
    close      = pyvmod.__dict__.get('vmod%s_close'      % device)

    T = 1.0/sampling
    print "sampling at %6.2fHz, T=%6.5f sec" % (sampling, T)

    h = get_handle(lun)
    if h < 0:
        print "could not open lun %d" % lun
        sys.exit(1)
    samples = []
    ticks = 0
    val = pyvmod.new_intp()
    while ticks < number_of_samples:
        ticks += 1
        err = convert(h, channel, ampli, val)
        value_read = dac(pyvmod.intp_value(val))
        if err < 0:
            print "conversion failed for channel %d" % channel
            sys.exit(1)
        samples.append(value_read)
        time.sleep(T)
    pyvmod.delete_intp(val)
    close(h)

    out = file(outputfile, 'w')
    for v in samples:
        out.write("%6.4f\n" % v)
    out.close()

