#!/bin/sh

vmedesc $* --format=flat SIS3300 | ./sis33inst.pl sis3300
