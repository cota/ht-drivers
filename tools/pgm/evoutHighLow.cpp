
0:

loop:
      movv 0x14010203 EVOUTH
      movr EVOUTFB     lr1
      movv 0x14010203 EVOUTL
      movr EVOUTFB     lr2
      jmp loop
      
