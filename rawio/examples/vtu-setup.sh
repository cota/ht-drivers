#!/bin/bash

BA=0xb00000

function ba () {
    echo "obase=16; $(( $BA + $1 ))" | bc
}

#1.1 Stop VTU and enable writing in both context register
vmeio w $(ba 0x0) 0x39 16 -- 0x3010
#1.2
vmeio w $(ba 0x3a) 0x39 16 -- 0x0
vmeio w $(ba 0x3c) 0x39 16 -- 0x0
vmeio w $(ba 0x3e) 0x39 16 -- 0x8b38
vmeio w $(ba 0x40) 0x39 16 -- 0x0
vmeio w $(ba 0x42) 0x39 16 -- 0x0
vmeio w $(ba 0x44) 0x39 16 -- 0x8b38
vmeio w $(ba 0x46) 0x39 16 -- 0x0
vmeio w $(ba 0x48) 0x39 16 -- 0x0

#2 SwapEnaOvr, Counting Enable, DblReg disabled, Sync op
vmeio w $(ba 0x0) 0x39 16 -- 0xff61

#3 Write Trigger Selector (Start, Stop, Restart)
vmeio w $(ba 0xe) 0x39 16 -- 0xff01
vmeio w $(ba 0x10) 0x39 16 -- 0xffff

#4 Write control register 3 (Clear Faults only)
vmeio w $(ba 0xc) 0x39 16 -- 0xff80

#5 Write control register 2 (No sync, stop and start, Refresh Status)
vmeio w $(ba 0xa) 0x39 16 -- 0xffa2

#6 Write control register 2 (No sync, stop and start, Refresh Status)
vmeio w $(ba 0x5a) 0x39 16 -- 0xff20

#7 Write control register 5 (IntlkFreqMeasDis=true, IntlkGeneralDis=false, 11A interlock is used)
vmeio w $(ba 0x62) 0x39 16 -- 0xff08

#8 Set interrupt level
vmeio w $(ba 0x4) 0x39 16 -- 0x2