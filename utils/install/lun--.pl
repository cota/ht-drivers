#!/usr/bin/perl
#
# lun--.pl
# Convert an XML file by decreasing the logical_module_number of certain
# device types. The result is printed to stdout.
#
# The device types taken as input are non-case-sensitive.
# If no device types are given, the original XML is printed verbatim.
#
# Example:
# $ lun--.pl --devices=mydev,ANOTHER_DEVICE
# decrements the logical_module_number of all 'mydev' and 'another_device'
# (remember: non-case-sensitive!) modules in /etc/drivers.xml.
#
# This is all a quick hack: this should be done with an XML parser (imagine
# the XML file didn't have any line breaks!), but we don't have XML modules
# available on our front-ends.
#

use strict;
use warnings;
use Getopt::Long;

my @devices;

GetOptions("devices=s" => \@devices) or die("Failed to parse arguments\n");

# allow comma-separated input
@devices = split(/,/, join(',', @devices));

# build the string that will match the given devices
my $match;
foreach my $device (@devices) {
    if (defined $match) {
	$match = $match.'|'.$device;
    } else {
	$match = $device;
    }
}

my $filename = '/etc/drivers.xml';

open(INPUT, "<$filename") or die ("$filename not found");

LINE: while (<INPUT>) {
    if (@devices == 0 or $_ !~ m|<module| or $_ !~ m|name="($match)"|i) {
	print $_;
	next LINE;
    }

    my $field = 'logical_module_number';
    m/$field="(\d+)"/;

    my $val = $1;
    my $lun = $val > 0 ? $val - 1 : $val;
    $_ =~ s/$field="$1"/$field="$lun"/;

    print $_;
}
close(INPUT);
