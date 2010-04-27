#!/usr/bin/perl

# take the value of the SPI register and decode it into a human-readable form
#
# the arguments of this script are the values of the SPI register. Example:
#
# ./spi.pl 0x7008107D 0x70081101
# Input: 0x7008107d
# 31: CLK: local 40MHz
# 30: SYNC: no sync
# 29: RESET: No
# 28: POWDOWN: No
# 27: STATUS (R/O): 0
# 26: REFMON (R/O): 0
# 25: LOCKDETECT (R/O): 0
# 20: DATAUP2DATE: 0
# 19: SENDSPI: Send frame (bits 18-0)
# 18: AD-R/W: Write
# 17-8: Address: 0x1000 (4096)
# 7-0: Data: 0x7d (125)
# ---------------------------
# Input: 0x70081101
# 31: CLK: local 40MHz
# 30: SYNC: no sync
# 29: RESET: No
# 28: POWDOWN: No
# 27: STATUS (R/O): 0
# 26: REFMON (R/O): 0
# 25: LOCKDETECT (R/O): 0
# 20: DATAUP2DATE: 0
# 19: SENDSPI: Send frame (bits 18-0)
# 18: AD-R/W: Write
# 17-8: Address: 0x1100 (4352)
# 7-0: Data: 0x1 (1)

%addresses = (
    0x00 => "Serial Port Configuration",
    0x01 => "Blank",
    0x02 => "Reserved",
    0x03 => "Reserved",
    0x04 => "Read Back Control",
    0x10 => "PFD and Charge Pump",
    0x11 => "R Counter LSB",
    0x12 => "R Counter MSB",
    0x13 => "A Counter",
    0x14 => "B Counter LSB",
    0x15 => "B Counter MSB",
    0x16 => "PLL Control 1",
    0x17 => "PLL Control 2",
    0x18 => "PLL Control 3",
    0x19 => "PLL Control 4",
    0x1A => "PLL Control 5",
    0x1B => "PLL Control 6",
    0x1C => "PLL Control 7",
    0x1D => "PLL Control 8",
    0x1E => "PLL Control 9",
    0x1F => "PLL Readback",
# 0x20 to 0x4F is blank

# Fine Delay Adjust: OUT6 to OUT9
    0xA0 => "OUT6 Delay Bypass",
    0xA1 => "OUT6 Delay Full-Scale",
    0xA2 => "OUT6 Delay Fraction",
    0xA3 => "OUT7 Delay Bypass",
    0xA4 => "OUT7 Delay Full-Scale",
    0xA5 => "OUT7 Delay Fraction",
    0xA6 => "OUT8 Delay Bypass",
    0xA7 => "OUT8 Delay Full-Scale",
    0xA8 => "OUT8 Delay Fraction",
    0xA9 => "OUT9 Delay Bypass",
    0xAA => "OUT9 Delay Full-Scale",
    0xAB => "OUT9 Delay Fraction",
# 0xAC to 0xEF is blank

# LVPECL Outputs
    0xF0 => "LVPECL OUT0",
    0xF1 => "LVPECL OUT1",
    0xF2 => "LVPECL OUT2",
    0xF3 => "LVPECL OUT3",
    0xF4 => "LVPECL OUT4",
    0xF5 => "LVPECL OUT5",
# 0xF6 to 0x13F is blank

# LVDS/CMOS Outputs
    0x140 => "LVDS/CMOS OUT6",
    0x141 => "LVDS/CMOS OUT7",
    0x142 => "LVDS/CMOS OUT8",
    0x143 => "LVDS/CMOS OUT9",
#0x144 to 0x18F is empty

# LVPECL Channel Dividers
    0x190 => "PECL Divider 0 (1/3)",
    0x191 => "PECL Divider 0 (2/3)",
    0x192 => "PECL Divider 0 (3/3)",
    0x193 => "PECL Divider 1 (1/3)",
    0x194 => "PECL Divider 1 (2/3)",
    0x195 => "PECL Divider 1 (3/3)",
    0x196 => "PECL Divider 2 (1/3)",
    0x197 => "PECL Divider 2 (2/3)",
    0x198 => "PECL Divider 2 (3/3)",
# LVDS/CMOS Channel Dividers
    0x199 => "LVDS/CMOS Divider 3 (1/5)",
    0x19A => "LVDS/CMOS Divider 3 (2/5)",
    0x19B => "LVDS/CMOS Divider 3 (3/5)",
    0x19C => "LVDS/CMOS Divider 3 (4/5)",
    0x19D => "LVDS/CMOS Divider 3 (5/5)",
    0x19E => "LVDS/CMOS Divider 4 (1/5)",
    0x19F => "LVDS/CMOS Divider 4 (2/5)",
    0x1A0 => "LVDS/CMOS Divider 4 (3/5)",
    0x1A1 => "LVDS/CMOS Divider 4 (4/5)",
    0x1A2 => "LVDS/CMOS Divider 4 (5/5)",
    0x1A3 => "Reserved",
# 0x1A4 to 0x1DF is blank

# VCO Divider and CLK Input
    0x1E0 => "VCO Divider",
    0x1E1 => "Input CLKs",
# 0x1E2 to 0x22A is blank

# System
    0x230 => "Power Down and Sync",
    0x231 => "Blank",

# Update All Registers
    0x232 => "Update All Registers"
    );

my $i = 0;

foreach my $arg_str (@ARGV) {

    ++$i;

    if (!($arg_str =~ /^0x/) && $arg_str !~ /^[0-9]/) {
	print "Comment: $arg_str\n";
	next;
    }

    if ($i > 1) {
	print "-----------------------------\n";
    }

    my $arg = hex($arg_str);
    printf "Input: 0x%x\n", $arg;

    bit31($arg);
    bit30($arg);
    bit29($arg);
    bit28($arg);
    bit27($arg);
    bit26($arg);
    bit25($arg);
    bit20($arg);
    bit19($arg);
    bit18($arg);
    address($arg);
    data($arg);
}

# reference clock
sub bit31 {
    print "31: CLK: ";
    if ($_[0] & (1 << 31)) {
	print "external";
    } else {
	print "local 40MHz";
    }
    print "\n";
}

# manual synchronisation
sub bit30 {
    print "30: SYNC: ";
    if ($_[0] & (1 << 30)) {
	print "no sync";
    } else {
	print "synchronise";
    }
    print "\n";
}

# reset AD
sub bit29 {
    print "29: RESET: ";
    if ($_[0] & (1 << 29)) {
	print "No";
    } else {
	print "Reset";
    }
    print "\n";
}

# power down
sub bit28 {
    print "28: POWDOWN: ";
    if ($_[0] & (1 << 29)) {
	print "No";
    } else {
	print "Power Down";
    }
    print "\n";
}

# status (read only)
sub bit27 {
    printf("27: STATUS (R/O): %d\n", !!($_[0] & (1 << 27)));
}

# ref monitor
sub bit26 {
    printf("26: REFMON (R/O): %d\n", !!($_[0] & (1 << 26)));
}

# lock detect
sub bit25 {
    printf("25: LOCKDETECT (R/O): %d\n", !!($_[0] & (1 << 25)));
}

# data up to date
sub bit20 {
    printf("20: DATAUP2DATE: %d\n", !!($_[0] & (1 << 20)));
}

# send SPI frame
sub bit19 {
    print "19: SENDSPI: ";
    if ($_[0] & (1 << 19)) {
	print "Send frame (bits 18-0)";
    } else {
	print "No";
    }
    print "\n";
}

# AD R/W
sub bit18 {
    print "18: AD-R/W: ";
    if ($_[0] & (1 << 18)) {
	print "Read";
    } else {
	print "Write";
    }
    print "\n";
}

# convert from decimal to binary
sub dec2bin {
    my $str = unpack("B32", pack("N", shift));
    $str =~ s/^0+(?=\d)//;   # otherwise you'll get leading zeros
    return $str;
}

# address
sub address {
    my $val = ($_[0] & 0x3FF00) >> 8;

    printf("17-8: Address: 0x%x", $val, $val);
    print " ($addresses{$val}) \n";
}

# data
sub data {
    my $val = $_[0] & 0xFF;

    printf("7-0: Data: 0x%x (%d) 0b%s\n", $val, $val, dec2bin($val));
}
