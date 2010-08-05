#!/usr/bin/perl
#
# Install an sis33xx module from a vmedesc(1) flat description

use strict;
use warnings;
use Getopt::Long;
use Pod::Usage;

sub module_is_loaded {
    my $module = shift;

    open(LSMOD, '/proc/modules') or die("Cannot open /proc/modules.\n");
    while (<LSMOD>) {
	chomp;
	my ($modname, $rest) = split(/\s+/, $_);

	if ($modname =~ m/^$module$/) {
	    close(LSMOD);
	    return 1;
	}
    }
    close(LSMOD);
    return 0;
}

sub get_param {
    my ($aref, $param) = @_;

    foreach (@$aref) {
	if ($_ =~ m/^$param=([^ ]+)$/) {
	    return $1;
	}
    }
    return undef;
}

# allow single-character options
Getopt::Long::Configure("bundling_override");
my $help = 0;
my $man = 0;

GetOptions(
    "help|?|h"		=> \$help,
    "man"		=> \$man,
    ) or pod2usage(2);
pod2usage(1) if $help;
pod2usage(-exitstatus => 0, -verbose => 2) if $man;

my $modname;

if (@ARGV == 0) {
    print STDERR "Missing kernel module name\n";
    pod2usage(2);
} else {
    $modname = $ARGV[0];
}

my @input;
while (<STDIN>) {
    push @input, split;
}
if (!@input) {
    die("No input via STDIN provided\n");
}

my $index	= get_param(\@input, 'lun');
my $base	= get_param(\@input, 'base1');
my $vector	= get_param(\@input, 'irqv');
my $level	= get_param(\@input, 'irql');

if (!defined $index or !defined $base or !defined $vector or !defined $level) {
    die("Invalid arguments\n");
}
if (!module_is_loaded('sis33')) {
    system('insmod sis33.ko') == 0 or die("'insmod sis33.ko' failed");
}

my $insmod = "insmod $modname.ko index=$index base=$base vector=$vector level=$level";
system($insmod) == 0 or die("$insmod failed");

__END__

=head1 NAME
sis33inst.pl - Install an sis33xx kernel module

=head1 SYNOPSIS

sis33inst.pl modname

sis33inst.pl - Install an sis33xx kernel module with the modparams given in STDIN.

=head1 DESCRIPTION

B<sis33inst.pl> loads an sis33xx kernel module into the kernel. It
reads the module's parameters from STDIN in vmedesc(1)'s flat format.

This program also installs sis33.ko if it is not already loaded.

=head1 AUTHOR

Written by Emilio G. Cota <cota@braap.org>

=cut
