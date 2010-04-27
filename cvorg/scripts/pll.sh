octave -q pll.m && \
cat pllplot.dat | sort -n | sed ':a;s/\(.*[0-9]\)\([0-9]\{3\}\)/\1,\2/;ta' > pllplotsorted.dat && \
cat pllplotsorted.dat | egrep '([a-zA-Z]|100,000,000)' > pllprint.dat
