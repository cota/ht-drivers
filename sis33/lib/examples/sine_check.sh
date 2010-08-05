#!/bin/bash
# sine_check.sh
# Checks that a device fed with a sinewave actually reads back a sinewave.
#
# Just feed an sis33 device with a sinewave of a known frequency
# Note that the frequency must be such that for all available
# frequencies and event lenghts, a sine-like wave can be recovered.
# For instance, if we feed it with a very slow signal, the acquired data
# will hardly resemble a sinewave, thus rendering the test useless.
#
# Besides the auto start/stop modes, manual start/stop modes can
# also be tested. Just set {start,stop}_auto to 0 before calling
# this script, and it will issue software start/stop triggers.
#
# NOTE: If the sampling clock shows significant wander, then the
# sampled waveform won't be an almost perfect sinusoidal, and
# thus the script will fail. Usually this shows up as a
# degradation of the result of the algorithm in this script
# with increasing event lengths.

CPU=L865

usage()
{
cat << EOF
`basename $0`: test an sis33 device with a given sinusoidal input
    Usage: `basename $0` [-h] [c<CHANNELS>] [-m<DEVICE_NUMBER>] [-w] input_freq (MHz)
      options:
        -c List of channels (default: 0)
	-h Show this message
	-m Device number (default: 0)
        -t Relative diff threshold. Max. acceptable relative difference between the wave's energy
           and the energy contained in the first harmonic. (default: 0.15)
        -w Exit when reldiff is first found to be greater than the threshold.
EOF
}

while getopts "c:hm:t:w" OPTION
do
    case $OPTION in
	c)
	    CHANNELS="$OPTARG"
	    ;;
	h)
	    usage
	    exit 1
	    ;;
	m)
	    DEV="$OPTARG"
	    ;;
	t)
	    THRESHOLD="$OPTARG"
	    ;;
	w)
	    EXIT_ON_WARN=1
	    ;;
	?)
	    usage
	    exit
	    ;;
    esac
done

# set subsequent non-option arguments to $1...n
shift $((OPTIND-1))
OPTIND=1

if [[ "$1" == "" ]]; then
    echo "An input frequency (in MHz) must be provided"
    usage
    exit 1
fi
INPUT_FREQ=$1

if [[ "$DEV" == "" ]]; then
    DEV=0
fi

if [[ "$CHANNELS" == "" ]]; then
    CHANNELS=0
fi

if [[ "$THRESHOLD" == "" ]]; then
    THRESHOLD=0.15
fi

set -e

TMPFILE=/tmp/plot.dat.$DEV
DEVDIR=/sys/class/sis33/sis33.$DEV

FREQS=$(available_freqs.$CPU -m$DEV | awk '{print $1}' | xargs)
EV_LENGTHS=$(available_event_lengths.$CPU -m$DEV -r | xargs)
EV_LEN_MAX=$(echo "$EV_LENGTHS" | cut -f1 -d' ')
N_CHANNELS=$(ls $DEVDIR/channels | wc -w)
N_SEGMENTS_MIN=$(cat $DEVDIR/n_segments_min)
N_SEGMENTS_MAX=$(cat $DEVDIR/n_segments_max)
DESC=$(cat $DEVDIR/description)
AUTOSTART=$(cat $DEVDIR/start_auto)
AUTOSTOP=$(cat $DEVDIR/stop_auto)

norm_evlen()
{
    evlen=$1

    N_SEGMENTS=$(cat $DEVDIR/n_segments)
    FACTOR=$((N_SEGMENTS/N_SEGMENTS_MIN))

    echo -n $((evlen/FACTOR))
}

send_stop()
{
    echo 0 > $DEVDIR/trigger
}

send_start()
{
    echo 1 > $DEVDIR/trigger
}

acq_cancel()
{
    echo 0 > $DEVDIR/acq_cancel
}

test_channel()
{
    segment=$1
    mychan=$2
    nr_events=$3
    evlen=$4

    ./acquire.$CPU -f -s$segment -c$mychan -l$(norm_evlen $evlen) -m$DEV -e$nr_events > $TMPFILE
    MYOUTPUT=$(awk "BEGIN {i=0;} {if(/^\$/) i++; if(i==$event && \$1) print \$1;}" $TMPFILE | \
	./fourier1.$CPU -o$INPUT_FREQ -s$freq)
    RELDIFF=$(echo "$MYOUTPUT" | grep 'reldiff' | awk '{print $2}')
    echo "$RELDIFF"
    if [[ "$RELDIFF" > "$THRESHOLD" ]]; then
	echo "Warning: event $event, evlen $evlen, freq $freq MHz, ch $chan, reldiff $RELDIFF too big:" 1>&2
	echo "$MYOUTPUT" 1>&2
	if [[ $EXIT_ON_WARN -eq 1 ]]; then
	    exit -1
	fi
    fi
}

acquire()
{
    segment=$1
    nr_events=$2
    evlen=$3
    params=$4

    ./acquire.$CPU -a -s$segment -l$(norm_evlen $evlen) -m$DEV -e$nr_events $params
}

fetch_test()
{
    segment=$1
    nr_events=$2
    evlen=$3

    for chan in $CHANNELS
    do
	echo "ch$chan"
	./acquire.$CPU -f -s$segment -c$chan -l$(norm_evlen $evlen) -m$DEV -e$nr_events > $TMPFILE
	for event in $( seq 0 $((nr_events-1)) )
	do
	    test_channel $segment $chan $nr_events $evlen
	done
    done
}

random_msleep()
{
    perl -e 'select (undef, undef, undef, int(rand()) % 10)'
}

do_random_trig()
{
    nr_events=$1

    for event in $( seq 0 $((nr_events-1)) )
    do
	if [[ $AUTOSTART -eq 0 ]]; then
	    send_start
	fi
	random_msleep
	if [[ $AUTOSTOP -eq 0 ]]; then
	    send_stop
	fi
    done
}

echo $DESC
# make sure there is no ongoing acquisition that could bother us
acq_cancel
for nr_segments in $( seq $N_SEGMENTS_MIN $N_SEGMENTS_MAX )
do
    echo "number of segments: $nr_segments / $N_SEGMENTS_MAX"
    echo "$nr_segments" > $DEVDIR/n_segments
    for segment in $( seq 0 $((nr_segments-1)) )
    do
	echo "segment $segment"
	for evlen in $EV_LENGTHS
	do
	    echo "ev_length $evlen"
	    nr_events=$((EV_LEN_MAX/evlen))
	    for freq in $FREQS
	    do
		./clock.$CPU -f$freq -s"internal" -m$DEV > /dev/null
		printf "======= freq %6gMHz =======\n" $freq

		if [[ $AUTOSTART -eq 0 || $AUTOSTOP -eq 0 ]]; then
		    params='-n'
		else
		    params=''
		fi

		acquire $segment $nr_events $evlen $params
		do_random_trig $nr_events

		fetch_test $segment $nr_events $evlen
	    done
	done
    done
done

