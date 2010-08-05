#!/usr/bin/perl
# offsets of the event directories of the sis3320

use integer;

my $i;
my $calc;
my $offset = 0x2010000;

for ($i = 0; $i < 8; $i++) {
    $calc = $offset;
    $calc += ($i/2) * 0x800000;
    $calc += ($i%2) * 0x8000;
    printf "%d\t0x%08x\n", $i, $calc;
}

