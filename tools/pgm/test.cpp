# 1 "wrEVOUTH.cpp"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "wrEVOUTH.cpp"

0:
   movv 0x14000005 EVOUTL
loop:
    wrlv 1 MSFR
    movv 0x14000000 EVOUTH
    movv 0x14111111 EVOUTL
    movv 0x14000000 EVOUTH
    movv 0x14000000 EVOUTH
    movv 0x14000000 EVOUTH
    movv 0x14000000 EVOUTH
    movv 0x14000000 EVOUTH
    movv 0x14000000 EVOUTH
    movv 0x14222222 EVOUTL
    jmp loop
