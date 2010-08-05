#!/bin/bash

CPU=L865
DEVICENR=1
SEGMENT=0
CHANNEL=0
EV_LENGTH=256
# use 'x11' as gnuplot to get the plot in X
GNUPLOT_TERM=dumb
SLEEPSECS=1
TMPFILE=/tmp/plot.dat

# this is the same as calling sis33_get_nr_bits(device)
N_BITS=$(cat /sys/class/sis33/sis33.$DEVICENR/n_bits)
let FULLSCALE=2**$N_BITS;

while true; do
    clear
    ./acquire.$CPU -s$SEGMENT -c$CHANNEL -l$EV_LENGTH -m$DEVICENR > $TMPFILE
    echo "set term $GNUPLOT_TERM; set yrange [0:$FULLSCALE]; plot '$TMPFILE' with linespoints; pause $SLEEPSECS;" \
	| gnuplot -noraise
# this sleep here allows us to kill the process
    sleep 0.7
done

