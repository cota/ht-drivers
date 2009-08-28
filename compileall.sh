echo "Compiling driver.."
cd driver; ./compiledrvr
echo "Compiling library.."
cd ../lib; make clean CPU=ppc4 && make all CPU=ppc4
echo "Compiling test program.."
cd ../test; make clean CPU=ppc4 && make all CPU=ppc4
exit 0
