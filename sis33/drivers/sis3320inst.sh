#!/bin/sh

vmedesc $* --format=flat SIS3320 | ./sis33inst.pl sis3320
