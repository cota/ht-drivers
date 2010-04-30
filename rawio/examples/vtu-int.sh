#!/bin/bash

BA=0xb00000

function ba () {
    echo "obase=16; $(( $BA + $1 ))" | bc
}

vmeio w $(ba 0x0) 0x39 16 -- 0x0202