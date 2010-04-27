# first compile, then install the whole thing

OBJ_DIR=object_cvorg

for COMPLEX in lab; do
	for CPU in L865 ppc4; do \
		DRV_DIR="/acc/local/$CPU/drv/cvorg"; \
		TEST_DIR="/acc/dsc/$COMPLEX/$CPU/bin"; \
		make -s CPU=$CPU && \
		cd driver && \
		make deliver $COMPLEX CPU=$CPU && \
		cd - && \
		dsc_install include/*.h			$DRV_DIR && \
		dsc_install lib/libcvorg.$CPU.a		$DRV_DIR && \
		dsc_install $OBJ_DIR/cvorgtest.$CPU	$TEST_DIR && \
		echo -e "\n\ncvorg installed OK for CPU=$CPU and COMPLEX=$COMPLEX\n\n";
	done
	dsc_install driver/cvorginstall.sh /acc/dsc/$COMPLEX/L865/2.6.24.7-rt27/cvorg && \
	dsc_install driver/cvorginstall.sh /acc/dsc/$COMPLEX/ppc4/4.0.0/cvorg
done

