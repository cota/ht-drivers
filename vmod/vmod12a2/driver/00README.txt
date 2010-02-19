00README.txt		This file
Makefile		A Makefile for building the whole thing locally
makemodules		Shell script specific for making the .ko module

vmod12a2.c		Driver source
vmod12a2.h
vmod12a2.ko		Driver object for pcgw39 kernel version
vmod12a2.xml		drivers.xml sample for tests

libvmod12a2.c		Library source
libvmod12a2.h
libvmod12a2.a		Library binary

pp.c			Bare-bones test program calling directly the driver
pplib.c			Bare-bones test program calling directly the library

drivers.xml		The real thing in pcgw39
transfer.ref		
vmod12a2install.sh	install script for deployment
parse.py		Install program
