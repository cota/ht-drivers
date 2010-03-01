#!	/bin/sh
insmod carrier.ko
./instprog.L865 pcgw23.xml MOD-PCI
python vmodparse.py pcgw23.xml vmod12a2 vmod12e16
