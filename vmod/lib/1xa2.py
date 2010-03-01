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

def time_for_next_tick(t, T):
    ret = t - T * math.floor(t/T)
    if ret > 0:
        return ret
    else:
        return T

def adc(decimal, resolution):
    return int ((1<<resolution) * (decimal+10.0)/20.0)

if __name__ == '__main__':

    parser = optparse.OptionParser()
    parser.add_option("-l", "--lun", dest="lun", type="int",
        help="logical module number", default=1)
    parser.add_option("-c", "--channel", dest="channel", type="int",
        help="channel (0 or 1)", default=0)
    parser.add_option("-p", "--period", dest="period", type="float",
        help="wave period in seconds", default=1.0)
    parser.add_option("-s", "--sampling", dest="sampling", type="float",
        help="sampling frequency in Hz", default=20)
    parser.add_option("-d", "--device", dest="device", type="string",
        help="device type (12a2, 16a2)", default="12a2")


    (options, args) = parser.parse_args()
    locals().update(options.__dict__)

    if device not in ['12a2', '16a2']:
        parser.error('--device must be either 12a2 or 16a2')
    get_handle = pyvmod.__dict__.get('vmod%s_get_handle' % device)
    convert    = pyvmod.__dict__.get('vmod%s_convert'    % device)
    close      = pyvmod.__dict__.get('vmod%s_close'      % device)
    resolution = 12
    if device == '16a2':
        resolution = 16

    T = 1.0/sampling

    ticks = 0
    h1 = get_handle(lun)
    if h1 < 0:
        print "could not open lun %d" % lun
        sys.exit(1)
    while True:
        ticks += 1
        val = wave(ticks*T, period)
        err = convert(h1, channel, adc(val, resolution))
        if err < 0:
            print "conversion failed for channel %d" % channel
            sys.exit(1)
        time.sleep(T)
        # print t, ticks, adc(wave(ticks*T,period))
    close(h1)
