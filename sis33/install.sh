# first compile, then install the whole thing

KVER=2.6.24.7-rt27

for COMPLEX in lab; do
    for CPU in L865; do
	DRV_DIR="/acc/dsc/$COMPLEX/$CPU/$KVER/sis33"
	LIB_DIR="/acc/local/$CPU/drv/sis33"
	TEST_DIR="/acc/dsc/$COMPLEX/$CPU/bin"
	make -s CPU=$CPU KVER=$KVER && \
	    cd drivers && \
	    dsc_install *.ko sis33inst.pl sis33{2,0}0inst.sh $DRV_DIR && \
	    cd - && \
	    dsc_install include/sis33.h include/libsis33.h $LIB_DIR && \
	    dsc_install lib/libsis33.$CPU.a		$LIB_DIR && \
	    dsc_install test/sis33test.$CPU		$TEST_DIR && \
	    echo -e "\n\nsis33 installed OK on '$COMPLEX' for CPU=$CPU and KVER=$KVER\n\n";
    done
done

