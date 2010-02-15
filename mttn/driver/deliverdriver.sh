#!/bin/bash
#
# little bricolage to deliver the driver plus its header files

DELIVERY_DIR=mttn
HEADERS=(mtt.h mtthard.h)

case "$1" in
    "ppc4" )
	;;
    * )
	echo -n "`basename $0`: deliver the driver "
	echo    "and header to lab, oper and oplhc"
	echo "Usage: `basename $0` CPU"
	echo "  options:"
	echo "  CPU={ppc4}"
	echo ""
	echo "Example:"
	echo "\$ `basename $0` ppc4"
	exit 1
esac

CPU=$1

set -e

# compile first
./compiledrvr

# deliver the driver
make deliver CPU=$CPU lab
make deliver CPU=$CPU oper
make deliver CPU=$CPU oplhc

# deliver header files
for header in "${HEADERS[@]}"
do
    echo -n "delivering ${header} to /ps/local/$CPU/$DELIVERY_DIR.. "
    dsc_install ${header} /ps/local/$CPU/$DELIVERY_DIR
    echo "OK"
done
