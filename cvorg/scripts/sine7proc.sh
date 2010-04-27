cat $1 | sed 's/: /:\t/' | sed 's/=/=\t/' | cut -f3 | grep -E '[0-9]' > sine7.dat

