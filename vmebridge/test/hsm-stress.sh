#!/bin/bash
#
# Launch a group of processes all accessing concurrently the HSM module.

DURATION=5
TASKS=1
CPU='L865'

USAGE="`basename $0`: stress-test the HSM VME memory module.\n \
    Usage: `basename $0` -d<seconds> -n<#tasks>\n \
      options:\n \
      d: Duration of the test, in seconds (default: $DURATION.)\n \
      n: Number of concurrent tasks accessing the HSM (default: $TASKS.)\n";

while getopts d:n: c
do
    case $c in
	d)
	    DURATION=$OPTARG;;
	n)
	    TASKS=$OPTARG;;
	\?)
	    printf "$USAGE";
	    exit 2;
    esac
done

for i in $(seq $TASKS)
do
    echo "executing task $i"
    ./hsm-dma.$CPU $DURATION &
done

let DURATION++
sleep $DURATION
exit 0
